// ProcessModule.hpp
#ifndef MODULES_PROCESSMODULE_HPP
#define MODULES_PROCESSMODULE_HPP

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing process exec and pipe-based I/O.
 *
 * Critical design rules (per docs/build-recipe-modules.md):
 * - argv is always a string array, never a single shell-parsed string.
 * - env is a complete map; if absent, child inherits parent env. env_clear
 *   wipes the inherited environment first.
 * - cwd defaults to the parent's cwd.
 * - Exit codes are integers; process_check throws on non-zero.
 *
 * process_run(program, argv, options?)            -> { exit_code, stdout, stderr, signal }
 * process_check(program, argv, options?)          -> stdout (throws on non-zero exit)
 * process_spawn(program, argv, options?)          -> handle (string)
 * process_write_stdin(handle, data)               -> null
 * process_read_stdout_line(handle)                -> string (empty string on EOF)
 * process_wait(handle)                            -> { exit_code, signal }
 * process_kill(handle, signal)                    -> null
 */
class ProcessModule : public BaseModule {
  public:
    ProcessModule() {
        setModuleName("Process");
        setDescription("Spawn external programs with argv arrays (no shell). Capture stdout/stderr, "
                       "control env/cwd/timeout, or hold a streaming handle for incremental I/O.");
        setBuiltIn(true);
    }

    struct SpawnHandle {
        pid_t pid       = -1;
        int   stdin_fd  = -1;
        int   stdout_fd = -1;
        int   stderr_fd = -1;
        bool  reaped    = false;
        int   exit_code = -1;
        int   term_sig  = 0;
        std::string stdout_buf;
    };

    using HandleMap = std::unordered_map<std::string, std::shared_ptr<SpawnHandle>>;

    static HandleMap & handles() {
        static HandleMap m;
        return m;
    }

    static std::mutex & handles_mutex() {
        static std::mutex m;
        return m;
    }

    static std::string nextHandleId() {
        static std::atomic<uint64_t> ctr{ 0 };
        return std::string("proc#") + std::to_string(ctr.fetch_add(1));
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> run_params = {
            { "program", Symbols::Variables::Type::STRING, "Program to execute (looked up via PATH if not absolute)", false, false },
            { "argv",    Symbols::Variables::Type::OBJECT, "Object/array of string arguments (does NOT include program)", false, false },
            { "options", Symbols::Variables::Type::OBJECT, "Optional: { cwd, env, env_clear, stdin, timeout_sec }", true,  false }
        };

        REGISTER_FUNCTION("process_run", Symbols::Variables::Type::OBJECT, run_params,
                          "Run a program synchronously and return { exit_code, stdout, stderr, signal }",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doRun(args, false);
                          });

        REGISTER_FUNCTION("process_check", Symbols::Variables::Type::STRING, run_params,
                          "Run a program synchronously. Return its stdout. Throws if exit code is non-zero.",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doRun(args, true);
                          });

        REGISTER_FUNCTION("process_spawn", Symbols::Variables::Type::STRING, run_params,
                          "Spawn a program asynchronously and return a handle string for streaming I/O",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doSpawn(args);
                          });

        std::vector<Symbols::FunctionParameterInfo> hd_param = {
            { "handle", Symbols::Variables::Type::STRING, "Handle returned by process_spawn", false, false }
        };
        std::vector<Symbols::FunctionParameterInfo> hd_data = {
            { "handle", Symbols::Variables::Type::STRING, "Handle returned by process_spawn", false, false },
            { "data",   Symbols::Variables::Type::STRING, "Bytes to write to the child's stdin", false, false }
        };
        std::vector<Symbols::FunctionParameterInfo> hd_sig = {
            { "handle", Symbols::Variables::Type::STRING,  "Handle returned by process_spawn", false, false },
            { "signal", Symbols::Variables::Type::INTEGER, "POSIX signal number (e.g. 9, 15)",  false, false }
        };

        REGISTER_FUNCTION("process_write_stdin", Symbols::Variables::Type::NULL_TYPE, hd_data,
                          "Write data to the spawned process's stdin",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doWriteStdin(args);
                          });

        REGISTER_FUNCTION("process_read_stdout_line", Symbols::Variables::Type::STRING, hd_param,
                          "Read one line (terminated by \\n) from the spawned process's stdout. Empty string at EOF.",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doReadLine(args);
                          });

        REGISTER_FUNCTION("process_wait", Symbols::Variables::Type::OBJECT, hd_param,
                          "Block until the spawned process exits and return { exit_code, signal }",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doWait(args);
                          });

        REGISTER_FUNCTION("process_kill", Symbols::Variables::Type::NULL_TYPE, hd_sig,
                          "Send a signal to the spawned process",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              return ProcessModule::doKill(args);
                          });
    }

    // ---- helpers --------------------------------------------------------

    static std::vector<std::string> argvFromValue(const Symbols::ValuePtr & v) {
        std::vector<std::string> out;
        if (v != Symbols::Variables::Type::OBJECT) {
            throw std::runtime_error("argv must be an object/array of strings");
        }
        const auto & m = v->get<Symbols::ObjectMap>();

        // Detect array-literal shape: every key parses as a non-negative integer.
        // If so, sort by integer value so [a, b, ..., m] iterates in insertion
        // order (not lexicographic, which would put "10" before "2"). Otherwise
        // fall back to map order (already lex-sorted by std::map).
        bool all_numeric = !m.empty();
        std::vector<std::pair<long long, const Symbols::ValuePtr*>> indexed;
        indexed.reserve(m.size());
        for (const auto & kv : m) {
            char *end = nullptr;
            long long n = std::strtoll(kv.first.c_str(), &end, 10);
            if (end == kv.first.c_str() || *end != '\0' || n < 0) {
                all_numeric = false;
                break;
            }
            indexed.emplace_back(n, &kv.second);
        }

        if (all_numeric) {
            std::sort(indexed.begin(), indexed.end(),
                      [](const auto & a, const auto & b) { return a.first < b.first; });
            for (const auto & [n, vp] : indexed) {
                if (*vp != Symbols::Variables::Type::STRING) {
                    throw std::runtime_error("argv element [" + std::to_string(n) +
                                             "] is not a string");
                }
                out.push_back((*vp)->get<std::string>());
            }
        } else {
            for (const auto & kv : m) {
                if (kv.second != Symbols::Variables::Type::STRING) {
                    throw std::runtime_error("argv element '" + kv.first + "' is not a string");
                }
                out.push_back(kv.second->get<std::string>());
            }
        }
        return out;
    }

    struct LaunchOpts {
        std::string                              cwd;
        bool                                     have_cwd       = false;
        std::vector<std::string>                 env_strings;
        bool                                     have_env       = false;
        bool                                     env_clear      = false;
        std::string                              stdin_data;
        bool                                     have_stdin     = false;
        int                                      timeout_sec    = 0;
    };

    static LaunchOpts parseOpts(const Symbols::ValuePtr & v) {
        LaunchOpts o;
        if (v == Symbols::Variables::Type::NULL_TYPE) {
            return o;
        }
        if (v != Symbols::Variables::Type::OBJECT) {
            throw std::runtime_error("options must be an object");
        }
        const auto & m = v->get<Symbols::ObjectMap>();
        auto it = m.find("cwd");
        if (it != m.end() && it->second == Symbols::Variables::Type::STRING) {
            o.cwd      = it->second->get<std::string>();
            o.have_cwd = true;
        }
        it = m.find("env_clear");
        if (it != m.end() && it->second == Symbols::Variables::Type::BOOLEAN) {
            o.env_clear = it->second->get<bool>();
        }
        it = m.find("env");
        if (it != m.end() && it->second == Symbols::Variables::Type::OBJECT) {
            o.have_env = true;
            for (const auto & kv : it->second->get<Symbols::ObjectMap>()) {
                if (kv.second != Symbols::Variables::Type::STRING) {
                    throw std::runtime_error("env value for '" + kv.first + "' must be a string");
                }
                o.env_strings.push_back(kv.first + "=" + kv.second->get<std::string>());
            }
        }
        it = m.find("stdin");
        if (it != m.end() && it->second == Symbols::Variables::Type::STRING) {
            o.stdin_data = it->second->get<std::string>();
            o.have_stdin = true;
        }
        it = m.find("timeout_sec");
        if (it != m.end() && it->second == Symbols::Variables::Type::INTEGER) {
            o.timeout_sec = it->second->get<int>();
        }
        return o;
    }

    // Fork-and-exec primitive. Returns child pid; sets in/out/err pipe fds in
    // the parent (caller-provided). Pipe ownership: the *_parent fds belong
    // to the caller (close them); the child end is closed inside.
    static pid_t spawnChild(const std::string &              program,
                            const std::vector<std::string> & argv,
                            const LaunchOpts &               opts,
                            int *                            stdin_parent_w,
                            int *                            stdout_parent_r,
                            int *                            stderr_parent_r) {
        int in_pipe[2]  = { -1, -1 };
        int out_pipe[2] = { -1, -1 };
        int err_pipe[2] = { -1, -1 };
        if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
            throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
        }

        pid_t pid = ::fork();
        if (pid < 0) {
            throw std::runtime_error(std::string("fork failed: ") + std::strerror(errno));
        }

        if (pid == 0) {
            // child
            ::dup2(in_pipe[0], STDIN_FILENO);
            ::dup2(out_pipe[1], STDOUT_FILENO);
            ::dup2(err_pipe[1], STDERR_FILENO);
            ::close(in_pipe[0]);
            ::close(in_pipe[1]);
            ::close(out_pipe[0]);
            ::close(out_pipe[1]);
            ::close(err_pipe[0]);
            ::close(err_pipe[1]);

            if (opts.have_cwd) {
                if (::chdir(opts.cwd.c_str()) != 0) {
                    ::_exit(127);
                }
            }

            std::vector<char *> cargv;
            cargv.reserve(argv.size() + 2);
            std::string prog_owned = program;
            cargv.push_back(prog_owned.data());
            std::vector<std::string> owned_args = argv;
            for (auto & s : owned_args) {
                cargv.push_back(s.data());
            }
            cargv.push_back(nullptr);

            std::vector<char *> cenv;
            std::vector<std::string> env_owned = opts.env_strings;
            if (opts.have_env) {
                for (auto & s : env_owned) {
                    cenv.push_back(s.data());
                }
                cenv.push_back(nullptr);
                ::execvpe(program.c_str(), cargv.data(), cenv.data());
            } else if (opts.env_clear) {
                cenv.push_back(nullptr);
                ::execvpe(program.c_str(), cargv.data(), cenv.data());
            } else {
                ::execvp(program.c_str(), cargv.data());
            }
            ::_exit(127);
        }

        // parent
        ::close(in_pipe[0]);
        ::close(out_pipe[1]);
        ::close(err_pipe[1]);
        if (stdin_parent_w) {
            *stdin_parent_w = in_pipe[1];
        } else {
            ::close(in_pipe[1]);
        }
        if (stdout_parent_r) {
            *stdout_parent_r = out_pipe[0];
        } else {
            ::close(out_pipe[0]);
        }
        if (stderr_parent_r) {
            *stderr_parent_r = err_pipe[0];
        } else {
            ::close(err_pipe[0]);
        }
        return pid;
    }

    // Drain child's stdout/stderr while feeding stdin, with optional timeout.
    static void pumpStdio(pid_t         pid,
                          int           stdin_fd,
                          int           stdout_fd,
                          int           stderr_fd,
                          const std::string & stdin_data,
                          std::string & out,
                          std::string & err,
                          int           timeout_sec,
                          int &         exit_code,
                          int &         term_sig) {
        size_t stdin_off = 0;
        bool   stdin_done = stdin_data.empty();
        if (stdin_done && stdin_fd >= 0) {
            ::close(stdin_fd);
            stdin_fd = -1;
        }
        const auto deadline_offset = timeout_sec > 0 ? timeout_sec : 0;
        const auto start_time      = ::time(nullptr);
        char       buf[4096];

        while (stdout_fd >= 0 || stderr_fd >= 0) {
            fd_set         rfds, wfds;
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            int max_fd = -1;
            if (stdout_fd >= 0) {
                FD_SET(stdout_fd, &rfds);
                max_fd = std::max(max_fd, stdout_fd);
            }
            if (stderr_fd >= 0) {
                FD_SET(stderr_fd, &rfds);
                max_fd = std::max(max_fd, stderr_fd);
            }
            if (!stdin_done && stdin_fd >= 0) {
                FD_SET(stdin_fd, &wfds);
                max_fd = std::max(max_fd, stdin_fd);
            }
            struct timeval tv;
            struct timeval * tvp = nullptr;
            if (deadline_offset > 0) {
                long elapsed = static_cast<long>(::time(nullptr) - start_time);
                long remain  = static_cast<long>(deadline_offset) - elapsed;
                if (remain <= 0) {
                    ::kill(pid, SIGKILL);
                    break;
                }
                tv.tv_sec  = remain;
                tv.tv_usec = 0;
                tvp        = &tv;
            }
            int rc = ::select(max_fd + 1, &rfds, &wfds, nullptr, tvp);
            if (rc < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            if (rc == 0) {
                ::kill(pid, SIGKILL);
                break;
            }
            if (stdout_fd >= 0 && FD_ISSET(stdout_fd, &rfds)) {
                ssize_t n = ::read(stdout_fd, buf, sizeof(buf));
                if (n <= 0) {
                    ::close(stdout_fd);
                    stdout_fd = -1;
                } else {
                    out.append(buf, static_cast<size_t>(n));
                }
            }
            if (stderr_fd >= 0 && FD_ISSET(stderr_fd, &rfds)) {
                ssize_t n = ::read(stderr_fd, buf, sizeof(buf));
                if (n <= 0) {
                    ::close(stderr_fd);
                    stderr_fd = -1;
                } else {
                    err.append(buf, static_cast<size_t>(n));
                }
            }
            if (!stdin_done && stdin_fd >= 0 && FD_ISSET(stdin_fd, &wfds)) {
                ssize_t n = ::write(stdin_fd,
                                    stdin_data.data() + stdin_off,
                                    stdin_data.size() - stdin_off);
                if (n <= 0) {
                    stdin_done = true;
                    ::close(stdin_fd);
                    stdin_fd = -1;
                } else {
                    stdin_off += static_cast<size_t>(n);
                    if (stdin_off >= stdin_data.size()) {
                        stdin_done = true;
                        ::close(stdin_fd);
                        stdin_fd = -1;
                    }
                }
            }
        }

        if (stdin_fd >= 0) {
            ::close(stdin_fd);
        }
        if (stdout_fd >= 0) {
            ::close(stdout_fd);
        }
        if (stderr_fd >= 0) {
            ::close(stderr_fd);
        }

        int status = 0;
        ::waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            term_sig  = 0;
        } else if (WIFSIGNALED(status)) {
            exit_code = -1;
            term_sig  = WTERMSIG(status);
        } else {
            exit_code = -1;
            term_sig  = 0;
        }
    }

    static Symbols::ValuePtr doRun(Symbols::FunctionArguments & args, bool throw_on_error) {
        if (args.size() < 2 || args.size() > 3 ||
            args[0] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error(std::string(throw_on_error ? "process_check" : "process_run") +
                                     " expects (string program, array argv, object options?)");
        }
        const std::string program = args[0]->get<std::string>();
        auto              argv    = argvFromValue(args[1]);
        LaunchOpts        opts;
        if (args.size() == 3) {
            opts = parseOpts(args[2]);
        }

        int stdin_w  = -1;
        int stdout_r = -1;
        int stderr_r = -1;
        pid_t pid    = spawnChild(program, argv, opts,
                                  opts.have_stdin ? &stdin_w : nullptr,
                                  &stdout_r, &stderr_r);

        std::string out, err;
        int         exit_code = -1;
        int         term_sig  = 0;
        pumpStdio(pid,
                  opts.have_stdin ? stdin_w : -1,
                  stdout_r, stderr_r,
                  opts.have_stdin ? opts.stdin_data : std::string(),
                  out, err,
                  opts.timeout_sec,
                  exit_code, term_sig);

        if (throw_on_error && exit_code != 0) {
            throw std::runtime_error("process_check: '" + program +
                                     "' exited with code " + std::to_string(exit_code) +
                                     (err.empty() ? "" : ("\n" + err)));
        }

        if (throw_on_error) {
            return Symbols::ValuePtr(out);
        }

        Symbols::ObjectMap result;
        int                ec_val  = exit_code;
        int                sig_val = term_sig;
        result["exit_code"] = Symbols::ValuePtr(ec_val);
        result["stdout"]    = Symbols::ValuePtr(out);
        result["stderr"]    = Symbols::ValuePtr(err);
        result["signal"]    = Symbols::ValuePtr(sig_val);
        return Symbols::ValuePtr(result);
    }

    static Symbols::ValuePtr doSpawn(Symbols::FunctionArguments & args) {
        if (args.size() < 2 || args.size() > 3 ||
            args[0] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("process_spawn expects (string program, array argv, object options?)");
        }
        const std::string program = args[0]->get<std::string>();
        auto              argv    = argvFromValue(args[1]);
        LaunchOpts        opts;
        if (args.size() == 3) {
            opts = parseOpts(args[2]);
        }

        auto h        = std::make_shared<SpawnHandle>();
        int  stdin_w  = -1;
        int  stdout_r = -1;
        int  stderr_r = -1;
        h->pid = spawnChild(program, argv, opts, &stdin_w, &stdout_r, &stderr_r);
        h->stdin_fd  = stdin_w;
        h->stdout_fd = stdout_r;
        h->stderr_fd = stderr_r;

        std::string id = nextHandleId();
        {
            std::lock_guard<std::mutex> lock(handles_mutex());
            handles()[id] = h;
        }
        return Symbols::ValuePtr(id);
    }

    static std::shared_ptr<SpawnHandle> getHandle(const std::string & id) {
        std::lock_guard<std::mutex> lock(handles_mutex());
        auto it = handles().find(id);
        if (it == handles().end()) {
            throw std::runtime_error("process: invalid or already-released handle: " + id);
        }
        return it->second;
    }

    static Symbols::ValuePtr doWriteStdin(Symbols::FunctionArguments & args) {
        if (args.size() != 2 ||
            args[0] != Symbols::Variables::Type::STRING ||
            args[1] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("process_write_stdin expects (string handle, string data)");
        }
        auto              h    = getHandle(args[0]->get<std::string>());
        const std::string data = args[1]->get<std::string>();
        if (h->stdin_fd < 0) {
            throw std::runtime_error("process_write_stdin: stdin already closed");
        }
        size_t off = 0;
        while (off < data.size()) {
            ssize_t n = ::write(h->stdin_fd, data.data() + off, data.size() - off);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error(std::string("process_write_stdin: ") + std::strerror(errno));
            }
            off += static_cast<size_t>(n);
        }
        return Symbols::ValuePtr::null();
    }

    static Symbols::ValuePtr doReadLine(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("process_read_stdout_line expects one string handle");
        }
        auto h = getHandle(args[0]->get<std::string>());
        if (h->stdout_fd < 0 && h->stdout_buf.empty()) {
            return Symbols::ValuePtr(std::string());
        }
        char buf[1024];
        while (true) {
            auto nl = h->stdout_buf.find('\n');
            if (nl != std::string::npos) {
                std::string line = h->stdout_buf.substr(0, nl);
                h->stdout_buf.erase(0, nl + 1);
                return Symbols::ValuePtr(line);
            }
            if (h->stdout_fd < 0) {
                std::string rest = std::move(h->stdout_buf);
                h->stdout_buf.clear();
                return Symbols::ValuePtr(rest);
            }
            ssize_t n = ::read(h->stdout_fd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error(std::string("process_read_stdout_line: ") + std::strerror(errno));
            }
            if (n == 0) {
                ::close(h->stdout_fd);
                h->stdout_fd = -1;
                continue;
            }
            h->stdout_buf.append(buf, static_cast<size_t>(n));
        }
    }

    static Symbols::ValuePtr doWait(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("process_wait expects one string handle");
        }
        const std::string id = args[0]->get<std::string>();
        auto              h  = getHandle(id);
        if (!h->reaped) {
            if (h->stdin_fd >= 0) {
                ::close(h->stdin_fd);
                h->stdin_fd = -1;
            }
            int status = 0;
            ::waitpid(h->pid, &status, 0);
            if (WIFEXITED(status)) {
                h->exit_code = WEXITSTATUS(status);
                h->term_sig  = 0;
            } else if (WIFSIGNALED(status)) {
                h->exit_code = -1;
                h->term_sig  = WTERMSIG(status);
            }
            if (h->stdout_fd >= 0) {
                ::close(h->stdout_fd);
                h->stdout_fd = -1;
            }
            if (h->stderr_fd >= 0) {
                ::close(h->stderr_fd);
                h->stderr_fd = -1;
            }
            h->reaped = true;
        }
        Symbols::ObjectMap out;
        int                ec_val  = h->exit_code;
        int                sig_val = h->term_sig;
        out["exit_code"] = Symbols::ValuePtr(ec_val);
        out["signal"]    = Symbols::ValuePtr(sig_val);
        // Note: handle is intentionally retained after reap so process_wait()
        // is idempotent. Voidscript's interpreter calls functions twice when
        // their result is assigned to a typed variable, and the second call
        // must not fail.
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr doKill(Symbols::FunctionArguments & args) {
        if (args.size() != 2 ||
            args[0] != Symbols::Variables::Type::STRING ||
            args[1] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("process_kill expects (string handle, int signal)");
        }
        auto h = getHandle(args[0]->get<std::string>());
        int  s = args[1]->get<int>();
        if (::kill(h->pid, s) != 0) {
            throw std::runtime_error(std::string("process_kill: ") + std::strerror(errno));
        }
        return Symbols::ValuePtr::null();
    }
};

}  // namespace Modules

#endif  // MODULES_PROCESSMODULE_HPP
