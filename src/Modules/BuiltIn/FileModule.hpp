// FileModule.hpp
#ifndef MODULES_FILEMODULE_HPP
#define MODULES_FILEMODULE_HPP

#include <fcntl.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "utils.h"

namespace Modules {

/**
 * @brief Module providing simple file I/O functions:
 *  file_get_contents(filename) -> string content
 *  file_put_contents(filename, content, overwrite) -> undefined, throws on error
 *  file_exists(filename) -> bool
 */
class FileModule : public BaseModule {

  public:
    FileModule() {
        setModuleName("File");
        setDescription("Provides file system operations including reading, writing, file existence checks, directory management, and file size queries");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        REGISTER_FUNCTION("file_get_contents", Symbols::Variables::Type::STRING, params, "Read the content of a file",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_get_contents expects 1 argument");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_get_contents expects string filename");
                              }
                              const std::string filename = args[0]->get<std::string>();
                              if (!utils::exists(filename)) {
                                  throw std::runtime_error("File does not exist: " + filename);
                              }
                              std::ifstream input(filename, std::ios::in | std::ios::binary);
                              if (!input.is_open()) {
                                  throw std::runtime_error("Could not open file: " + filename);
                              }
                              std::string content((std::istreambuf_iterator<char>(input)),
                                                  std::istreambuf_iterator<char>());
                              input.close();
                              return Symbols::ValuePtr(content);
                          });

        params = {
            { "file_name", Symbols::Variables::Type::STRING,  "The file name", false, false },
            { "content",   Symbols::Variables::Type::STRING,  "The content to write to the file", false, false },
            { "overwrite", Symbols::Variables::Type::BOOLEAN, "Whether to overwrite the file if it exists", false, false }
        };
        // Write content to file, with optional overwrite
        REGISTER_FUNCTION("file_put_contents", Symbols::Variables::Type::NULL_TYPE, params, "Write content into a file",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 3) {
                                  throw std::runtime_error("file_put_contents expects 3 arguments");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING ||
                                  args[1]->getType() != Symbols::Variables::Type::STRING ||
                                  args[2]->getType() != Symbols::Variables::Type::BOOLEAN) {
                                  throw std::runtime_error("file_put_contents expects (string, string, bool)");
                              }
                              const std::string filename  = args[0]->get<std::string>();
                              const std::string content   = args[1]->get<std::string>();
                              const bool        overwrite = args[2]->get<bool>();
                              if (!overwrite && utils::exists(filename)) {
                                  throw std::runtime_error("File already exists: " + filename);
                              }
                              std::ofstream output(filename, std::ios::out | std::ios::binary | std::ios::trunc);
                              if (!output.is_open()) {
                                  throw std::runtime_error("Could not open file for writing: " + filename);
                              }
                              output << content;
                              if (!output) {
                                  throw std::runtime_error("Failed to write to file: " + filename);
                              }
                              output.close();
                              return Symbols::ValuePtr::null();
                          });
        params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        // Check if file exists
        REGISTER_FUNCTION("file_exists", Symbols::Variables::Type::BOOLEAN, params, "Check if a file exists or not",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_exists expects 1 argument");
                              }
                              if (args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_exists expects string filename");
                              }
                              const std::string filename = args[0];
                              return utils::exists(filename);
                          });

        params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        REGISTER_FUNCTION("file_size", Symbols::Variables::Type::INTEGER, params, "Get the size of a file",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_size expects 1 argument");
                              }
                              if (args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_get_contents expects string filename");
                              }
                              const std::string filename = args[0]->get<std::string>();
                              if (utils::exists(filename) == false) {
                                  throw std::runtime_error("file_size: file not found: " + filename);
                              }
                              if (utils::is_directory((filename))) {
                                  return 4096;
                              }
                              size_t size = utils::file_size(filename);
                              return static_cast<int>(size);
                          });

        // Create directory with optional recursive creation
        params = {
            { "dir_path", Symbols::Variables::Type::STRING, "The directory path", false, false },
            { "recursive", Symbols::Variables::Type::BOOLEAN, "Whether to create parent directories recursively", true, false }
        };

        REGISTER_FUNCTION("mkdir", Symbols::Variables::Type::BOOLEAN, params, "Create a directory",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() < 1 || args.size() > 2) {
                                  throw std::runtime_error("mkdir expects 1 or 2 arguments");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("mkdir expects string directory path");
                              }
                              
                              const std::string dir_path = args[0]->get<std::string>();
                              bool recursive = false;
                              
                              if (args.size() == 2) {
                                  if (args[1]->getType() != Symbols::Variables::Type::BOOLEAN) {
                                      throw std::runtime_error("mkdir second argument must be boolean (recursive)");
                                  }
                                  recursive = args[1]->get<bool>();
                              }
                              
                              bool success;
                              if (recursive) {
                                  success = utils::create_directories(dir_path);
                              } else {
                                  success = utils::create_directory(dir_path);
                              }
                              
                              return Symbols::ValuePtr(success);
                          });

        // Remove directory (only if empty)
        params = {
            { "dir_path", Symbols::Variables::Type::STRING, "The directory path", false, false }
        };

        REGISTER_FUNCTION("rmdir", Symbols::Variables::Type::BOOLEAN, params, "Remove an empty directory",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("rmdir expects 1 argument");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("rmdir expects string directory path");
                              }

                              const std::string dir_path = args[0]->get<std::string>();
                              bool success = utils::remove_directory(dir_path);

                              return Symbols::ValuePtr(success);
                          });

        // file_unlink: remove a single file (idempotent — no-op if missing)
        std::vector<Symbols::FunctionParameterInfo> path_param = {
            { "path", Symbols::Variables::Type::STRING, "Path to the file", false, false }
        };
        REGISTER_FUNCTION("file_unlink", Symbols::Variables::Type::BOOLEAN, path_param,
                          "Remove a file. Idempotent — returns false if the file did not exist.",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_unlink expects one string argument");
                              }
                              std::error_code ec;
                              return Symbols::ValuePtr(std::filesystem::remove(std::string(args[0]), ec));
                          });

        // file_rm_rf: recursive idempotent delete (file or directory tree)
        REGISTER_FUNCTION("file_rm_rf", Symbols::Variables::Type::INTEGER, path_param,
                          "Recursively remove a path. Idempotent — returns the number of entries removed.",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_rm_rf expects one string argument");
                              }
                              std::error_code ec;
                              std::uintmax_t  count = std::filesystem::remove_all(std::string(args[0]), ec);
                              if (ec) {
                                  throw std::runtime_error("file_rm_rf failed: " + ec.message());
                              }
                              return static_cast<int>(count);
                          });

        // file_chmod
        std::vector<Symbols::FunctionParameterInfo> chmod_params = {
            { "path", Symbols::Variables::Type::STRING,  "Path to the file or directory",  false, false },
            { "mode", Symbols::Variables::Type::INTEGER, "POSIX permission bits (e.g. 0755)", false, false }
        };
        REGISTER_FUNCTION("file_chmod", Symbols::Variables::Type::NULL_TYPE, chmod_params,
                          "Change permission bits on a path",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::INTEGER) {
                                  throw std::runtime_error("file_chmod expects (string path, int mode)");
                              }
                              const std::string path = args[0];
                              const int         mode = args[1];
                              if (::chmod(path.c_str(), static_cast<mode_t>(mode)) != 0) {
                                  throw std::runtime_error("file_chmod failed for: " + path);
                              }
                              return Symbols::ValuePtr::null();
                          });

        // file_copy
        std::vector<Symbols::FunctionParameterInfo> copy_params = {
            { "src",     Symbols::Variables::Type::STRING, "Source path",                            false, false },
            { "dst",     Symbols::Variables::Type::STRING, "Destination path",                       false, false },
            { "options", Symbols::Variables::Type::OBJECT, "Optional: { recursive: bool, follow_symlinks: bool, overwrite: bool }", true, false }
        };
        REGISTER_FUNCTION("file_copy", Symbols::Variables::Type::NULL_TYPE, copy_params,
                          "Copy a file (or directory tree if recursive=true)",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() < 2 || args.size() > 3 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_copy expects (string src, string dst, object opts?)");
                              }
                              namespace fs = std::filesystem;
                              auto opts    = fs::copy_options::none;
                              if (args.size() == 3 && args[2] == Symbols::Variables::Type::OBJECT) {
                                  const auto & m = args[2]->get<Symbols::ObjectMap>();
                                  auto it = m.find("recursive");
                                  if (it != m.end() && it->second->get<bool>()) {
                                      opts |= fs::copy_options::recursive;
                                  }
                                  it = m.find("overwrite");
                                  if (it != m.end() && it->second->get<bool>()) {
                                      opts |= fs::copy_options::overwrite_existing;
                                  }
                                  it = m.find("follow_symlinks");
                                  if (it != m.end() && !it->second->get<bool>()) {
                                      opts |= fs::copy_options::copy_symlinks;
                                  }
                              }
                              std::error_code ec;
                              fs::copy(std::string(args[0]), std::string(args[1]), opts, ec);
                              if (ec) {
                                  throw std::runtime_error("file_copy failed: " + ec.message());
                              }
                              return Symbols::ValuePtr::null();
                          });

        // file_rename / move
        std::vector<Symbols::FunctionParameterInfo> rename_params = {
            { "src", Symbols::Variables::Type::STRING, "Source path",      false, false },
            { "dst", Symbols::Variables::Type::STRING, "Destination path", false, false }
        };
        REGISTER_FUNCTION("file_rename", Symbols::Variables::Type::NULL_TYPE, rename_params,
                          "Rename or move a file/directory",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_rename expects (string src, string dst)");
                              }
                              std::error_code ec;
                              std::filesystem::rename(std::string(args[0]), std::string(args[1]), ec);
                              if (ec) {
                                  throw std::runtime_error("file_rename failed: " + ec.message());
                              }
                              return Symbols::ValuePtr::null();
                          });

        // file_symlink
        std::vector<Symbols::FunctionParameterInfo> symlink_params = {
            { "target",   Symbols::Variables::Type::STRING, "Target the symlink will point to",       false, false },
            { "linkpath", Symbols::Variables::Type::STRING, "Path of the symlink itself",             false, false }
        };
        REGISTER_FUNCTION("file_symlink", Symbols::Variables::Type::NULL_TYPE, symlink_params,
                          "Create a symbolic link at linkpath pointing to target",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_symlink expects (string target, string linkpath)");
                              }
                              std::error_code ec;
                              std::filesystem::create_symlink(std::string(args[0]), std::string(args[1]), ec);
                              if (ec) {
                                  throw std::runtime_error("file_symlink failed: " + ec.message());
                              }
                              return Symbols::ValuePtr::null();
                          });

        // file_mtime
        REGISTER_FUNCTION("file_mtime", Symbols::Variables::Type::INTEGER, path_param,
                          "Return the modification time of a file as a Unix timestamp",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_mtime expects one string argument");
                              }
                              struct stat info;
                              if (::stat(std::string(args[0]).c_str(), &info) != 0) {
                                  throw std::runtime_error("file_mtime: cannot stat: " + std::string(args[0]));
                              }
                              return Symbols::ValuePtr(static_cast<int>(info.st_mtime));
                          });

        // file_set_mtime
        std::vector<Symbols::FunctionParameterInfo> set_mtime_params = {
            { "path",     Symbols::Variables::Type::STRING,  "Path to the file",       false, false },
            { "unix_ts",  Symbols::Variables::Type::INTEGER, "New mtime as unix epoch", false, false }
        };
        REGISTER_FUNCTION("file_set_mtime", Symbols::Variables::Type::NULL_TYPE, set_mtime_params,
                          "Set the modification time of a file (atime is set to the same value)",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::INTEGER) {
                                  throw std::runtime_error("file_set_mtime expects (string path, int unix_ts)");
                              }
                              const std::string path = args[0];
                              const int         ts   = args[1];
                              struct stat       info;
                              if (::stat(path.c_str(), &info) != 0) {
                                  throw std::runtime_error("file_set_mtime: cannot stat: " + path);
                              }
                              struct ::timespec times[2];
                              times[0].tv_sec  = info.st_atime;
                              times[0].tv_nsec = 0;
                              times[1].tv_sec  = static_cast<time_t>(ts);
                              times[1].tv_nsec = 0;
                              if (::utimensat(AT_FDCWD, path.c_str(), times, 0) != 0) {
                                  throw std::runtime_error("file_set_mtime failed for: " + path);
                              }
                              return Symbols::ValuePtr::null();
                          });

        // file_stat
        REGISTER_FUNCTION("file_stat", Symbols::Variables::Type::OBJECT, path_param,
                          "Return file metadata as { mode, uid, gid, size, mtime, type }",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_stat expects one string argument");
                              }
                              const std::string path = args[0];
                              struct stat       info;
                              if (::lstat(path.c_str(), &info) != 0) {
                                  throw std::runtime_error("file_stat: cannot stat: " + path);
                              }
                              std::string type = "unknown";
                              if (S_ISREG(info.st_mode)) {
                                  type = "file";
                              } else if (S_ISDIR(info.st_mode)) {
                                  type = "dir";
                              } else if (S_ISLNK(info.st_mode)) {
                                  type = "symlink";
                              } else if (S_ISFIFO(info.st_mode)) {
                                  type = "fifo";
                              } else if (S_ISSOCK(info.st_mode)) {
                                  type = "socket";
                              } else if (S_ISCHR(info.st_mode)) {
                                  type = "char_device";
                              } else if (S_ISBLK(info.st_mode)) {
                                  type = "block_device";
                              }
                              Symbols::ObjectMap out;
                              out["mode"]  = Symbols::ValuePtr(static_cast<int>(info.st_mode & 07777));
                              out["uid"]   = Symbols::ValuePtr(static_cast<int>(info.st_uid));
                              out["gid"]   = Symbols::ValuePtr(static_cast<int>(info.st_gid));
                              out["size"]  = Symbols::ValuePtr(static_cast<int>(info.st_size));
                              out["mtime"] = Symbols::ValuePtr(static_cast<int>(info.st_mtime));
                              out["type"]  = Symbols::ValuePtr(type);
                              return Symbols::ValuePtr(out);
                          });
    }
};

}  // namespace Modules
#endif  // MODULES_FILEMODULE_HPP
