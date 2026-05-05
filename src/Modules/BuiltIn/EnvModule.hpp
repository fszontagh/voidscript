// EnvModule.hpp
#ifndef MODULES_ENVMODULE_HPP
#define MODULES_ENVMODULE_HPP

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#else
extern "C" char ** environ;
#endif

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing environment-variable access.
 *
 * env_get(name)            -> string|null
 * env_has(name)            -> bool
 * env_set(name, value)     -> null
 * env_unset(name)          -> null
 * env_all()                -> object<string,string>
 * env_apply(map)           -> null
 */
class EnvModule : public BaseModule {
  public:
    EnvModule() {
        setModuleName("Env");
        setDescription("Provides access to process environment variables (read, write, unset, bulk apply)");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> name_param = {
            { "name", Symbols::Variables::Type::STRING, "Environment variable name", false, false }
        };

        REGISTER_FUNCTION("env_get", Symbols::Variables::Type::STRING, name_param,
                          "Get the value of an environment variable, or null if not set",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("env_get expects one string argument");
                              }
                              const std::string name = args[0];
                              const char * v = std::getenv(name.c_str());
                              if (v == nullptr) {
                                  return Symbols::ValuePtr::null();
                              }
                              return Symbols::ValuePtr(std::string(v));
                          });

        REGISTER_FUNCTION("env_has", Symbols::Variables::Type::BOOLEAN, name_param,
                          "Check whether an environment variable is set",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("env_has expects one string argument");
                              }
                              const std::string name = args[0];
                              return Symbols::ValuePtr(std::getenv(name.c_str()) != nullptr);
                          });

        std::vector<Symbols::FunctionParameterInfo> set_params = {
            { "name",  Symbols::Variables::Type::STRING, "Environment variable name",  false, false },
            { "value", Symbols::Variables::Type::STRING, "Environment variable value", false, false }
        };

        REGISTER_FUNCTION("env_set", Symbols::Variables::Type::NULL_TYPE, set_params,
                          "Set an environment variable for the current process",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 ||
                                  args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("env_set expects (string name, string value)");
                              }
                              const std::string name  = args[0];
                              const std::string value = args[1];
#ifdef _WIN32
                              if (SetEnvironmentVariableA(name.c_str(), value.c_str()) == 0) {
                                  throw std::runtime_error("env_set failed for: " + name);
                              }
                              _putenv_s(name.c_str(), value.c_str());
#else
                              if (::setenv(name.c_str(), value.c_str(), 1) != 0) {
                                  throw std::runtime_error("env_set failed for: " + name);
                              }
#endif
                              return Symbols::ValuePtr::null();
                          });

        REGISTER_FUNCTION("env_unset", Symbols::Variables::Type::NULL_TYPE, name_param,
                          "Unset an environment variable",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("env_unset expects one string argument");
                              }
                              const std::string name = args[0];
#ifdef _WIN32
                              SetEnvironmentVariableA(name.c_str(), nullptr);
                              _putenv_s(name.c_str(), "");
#else
                              ::unsetenv(name.c_str());
#endif
                              return Symbols::ValuePtr::null();
                          });

        std::vector<Symbols::FunctionParameterInfo> no_params = {};
        REGISTER_FUNCTION("env_all", Symbols::Variables::Type::OBJECT, no_params,
                          "Return all environment variables as an object<string,string>",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw std::runtime_error("env_all expects no arguments");
                              }
                              Symbols::ObjectMap out;
#ifdef _WIN32
                              LPCH block = GetEnvironmentStringsA();
                              if (block) {
                                  for (LPCH p = block; *p != '\0'; ) {
                                      std::string entry(p);
                                      auto eq = entry.find('=');
                                      if (eq != std::string::npos && eq > 0) {
                                          out[entry.substr(0, eq)] = Symbols::ValuePtr(entry.substr(eq + 1));
                                      }
                                      p += entry.size() + 1;
                                  }
                                  FreeEnvironmentStringsA(block);
                              }
#else
                              for (char ** e = environ; e && *e; ++e) {
                                  std::string entry(*e);
                                  auto eq = entry.find('=');
                                  if (eq != std::string::npos && eq > 0) {
                                      out[entry.substr(0, eq)] = Symbols::ValuePtr(entry.substr(eq + 1));
                                  }
                              }
#endif
                              return Symbols::ValuePtr(out);
                          });

        std::vector<Symbols::FunctionParameterInfo> apply_params = {
            { "map", Symbols::Variables::Type::OBJECT, "Object of name->value entries to set", false, false }
        };
        REGISTER_FUNCTION("env_apply", Symbols::Variables::Type::NULL_TYPE, apply_params,
                          "Bulk-set environment variables from an object map",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::OBJECT) {
                                  throw std::runtime_error("env_apply expects one object argument");
                              }
                              const auto & map = args[0]->get<Symbols::ObjectMap>();
                              for (const auto & kv : map) {
                                  if (kv.second != Symbols::Variables::Type::STRING) {
                                      throw std::runtime_error("env_apply: value for '" + kv.first +
                                                               "' must be a string");
                                  }
                                  const std::string value = kv.second->get<std::string>();
#ifdef _WIN32
                                  SetEnvironmentVariableA(kv.first.c_str(), value.c_str());
                                  _putenv_s(kv.first.c_str(), value.c_str());
#else
                                  ::setenv(kv.first.c_str(), value.c_str(), 1);
#endif
                              }
                              return Symbols::ValuePtr::null();
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_ENVMODULE_HPP
