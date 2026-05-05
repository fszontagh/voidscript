// PathModule.hpp
#ifndef MODULES_PATHMODULE_HPP
#define MODULES_PATHMODULE_HPP

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

namespace fs = std::filesystem;

/**
 * @brief Module providing path manipulation primitives backed by std::filesystem.
 *
 * path_join(base, ...parts)   -> string         (variadic via object/array)
 * path_dirname(p)             -> string
 * path_basename(p)            -> string
 * path_extension(p)           -> string
 * path_canonical(p)           -> string
 * path_exists(p)              -> bool
 * path_is_file(p)             -> bool
 * path_is_dir(p)              -> bool
 * path_is_symlink(p)          -> bool
 * path_glob(pattern)          -> object<int,string>
 * path_relative(from, to)     -> string
 */
class PathModule : public BaseModule {
  public:
    PathModule() {
        setModuleName("Path");
        setDescription("Provides path manipulation utilities (join, split, canonicalize, glob) backed by std::filesystem");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> single_path = {
            { "path", Symbols::Variables::Type::STRING, "A path string", false, false }
        };

        // path_join: base + variadic string parts (passed as an object<int,string>)
        std::vector<Symbols::FunctionParameterInfo> join_params = {
            { "base",  Symbols::Variables::Type::STRING, "Base path",                                    false, false },
            { "parts", Symbols::Variables::Type::OBJECT, "Object/array of additional path components",   true,  false }
        };
        REGISTER_FUNCTION("path_join", Symbols::Variables::Type::STRING, join_params,
                          "Join a base path with zero or more components using the platform separator",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.empty() || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_join expects a string base path");
                              }
                              fs::path p = std::string(args[0]);
                              for (size_t i = 1; i < args.size(); ++i) {
                                  if (args[i] == Symbols::Variables::Type::STRING) {
                                      p /= std::string(args[i]);
                                  } else if (args[i] == Symbols::Variables::Type::OBJECT) {
                                      const auto & map = args[i]->get<Symbols::ObjectMap>();
                                      for (const auto & kv : map) {
                                          if (kv.second != Symbols::Variables::Type::STRING) {
                                              throw std::runtime_error("path_join: parts must be strings");
                                          }
                                          p /= kv.second->get<std::string>();
                                      }
                                  } else {
                                      throw std::runtime_error("path_join: unsupported argument type");
                                  }
                              }
                              return Symbols::ValuePtr(p.string());
                          });

        REGISTER_FUNCTION("path_dirname", Symbols::Variables::Type::STRING, single_path,
                          "Return the parent directory portion of a path",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_dirname expects one string argument");
                              }
                              return Symbols::ValuePtr(fs::path(std::string(args[0])).parent_path().string());
                          });

        REGISTER_FUNCTION("path_basename", Symbols::Variables::Type::STRING, single_path,
                          "Return the final filename component of a path",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_basename expects one string argument");
                              }
                              return Symbols::ValuePtr(fs::path(std::string(args[0])).filename().string());
                          });

        REGISTER_FUNCTION("path_extension", Symbols::Variables::Type::STRING, single_path,
                          "Return the file extension including leading dot (e.g. \".gz\"), or empty string",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_extension expects one string argument");
                              }
                              return Symbols::ValuePtr(fs::path(std::string(args[0])).extension().string());
                          });

        REGISTER_FUNCTION("path_canonical", Symbols::Variables::Type::STRING, single_path,
                          "Resolve symlinks and \"..\" segments and return an absolute canonical path",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_canonical expects one string argument");
                              }
                              std::error_code ec;
                              fs::path        p   = std::string(args[0]);
                              fs::path        out = fs::weakly_canonical(p, ec);
                              if (ec) {
                                  throw std::runtime_error("path_canonical failed: " + ec.message());
                              }
                              return Symbols::ValuePtr(out.string());
                          });

        REGISTER_FUNCTION("path_exists", Symbols::Variables::Type::BOOLEAN, single_path,
                          "Return true if the path exists (file, dir, or symlink)",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_exists expects one string argument");
                              }
                              std::error_code ec;
                              return Symbols::ValuePtr(fs::exists(fs::path(std::string(args[0])), ec));
                          });

        REGISTER_FUNCTION("path_is_file", Symbols::Variables::Type::BOOLEAN, single_path,
                          "Return true if the path is a regular file",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_is_file expects one string argument");
                              }
                              std::error_code ec;
                              return Symbols::ValuePtr(fs::is_regular_file(fs::path(std::string(args[0])), ec));
                          });

        REGISTER_FUNCTION("path_is_dir", Symbols::Variables::Type::BOOLEAN, single_path,
                          "Return true if the path is a directory",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_is_dir expects one string argument");
                              }
                              std::error_code ec;
                              return Symbols::ValuePtr(fs::is_directory(fs::path(std::string(args[0])), ec));
                          });

        REGISTER_FUNCTION("path_is_symlink", Symbols::Variables::Type::BOOLEAN, single_path,
                          "Return true if the path is a symbolic link",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_is_symlink expects one string argument");
                              }
                              std::error_code ec;
                              return Symbols::ValuePtr(fs::is_symlink(fs::symlink_status(fs::path(std::string(args[0])), ec)));
                          });

        std::vector<Symbols::FunctionParameterInfo> glob_param = {
            { "pattern", Symbols::Variables::Type::STRING, "Glob pattern (supports * ? [..] in the final component)", false, false }
        };
        REGISTER_FUNCTION("path_glob", Symbols::Variables::Type::OBJECT, glob_param,
                          "Expand a glob pattern. Wildcards are matched in the final path component; the leading directory is taken literally.",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_glob expects one string argument");
                              }
                              const std::string pattern = std::string(args[0]);
                              fs::path          pp(pattern);
                              fs::path          dir   = pp.parent_path().empty() ? fs::path(".") : pp.parent_path();
                              std::string       glob  = pp.filename().string();

                              auto matches = [](const std::string & name, const std::string & pat) -> bool {
                                  size_t ni     = 0;
                                  size_t pi     = 0;
                                  size_t star_n = std::string::npos;
                                  size_t star_p = 0;
                                  while (ni < name.size()) {
                                      if (pi < pat.size() && (pat[pi] == '?' || pat[pi] == name[ni])) {
                                          ++ni;
                                          ++pi;
                                      } else if (pi < pat.size() && pat[pi] == '*') {
                                          star_p = pi++;
                                          star_n = ni;
                                      } else if (star_p != std::string::npos) {
                                          pi = star_p + 1;
                                          ni = ++star_n;
                                      } else {
                                          return false;
                                      }
                                  }
                                  while (pi < pat.size() && pat[pi] == '*') {
                                      ++pi;
                                  }
                                  return pi == pat.size();
                              };

                              Symbols::ObjectMap out;
                              std::error_code    ec;
                              if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
                                  return Symbols::ValuePtr(out);
                              }
                              size_t idx = 0;
                              for (const auto & e : fs::directory_iterator(dir, ec)) {
                                  if (matches(e.path().filename().string(), glob)) {
                                      out[std::to_string(idx++)] = Symbols::ValuePtr(e.path().string());
                                  }
                              }
                              return Symbols::ValuePtr(out);
                          });

        std::vector<Symbols::FunctionParameterInfo> rel_params = {
            { "from", Symbols::Variables::Type::STRING, "Base path",   false, false },
            { "to",   Symbols::Variables::Type::STRING, "Target path", false, false }
        };
        REGISTER_FUNCTION("path_relative", Symbols::Variables::Type::STRING, rel_params,
                          "Return the relative path from `from` to `to`",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("path_relative expects (string from, string to)");
                              }
                              std::error_code ec;
                              fs::path        result = fs::relative(std::string(args[1]), std::string(args[0]), ec);
                              if (ec) {
                                  throw std::runtime_error("path_relative failed: " + ec.message());
                              }
                              return Symbols::ValuePtr(result.string());
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_PATHMODULE_HPP
