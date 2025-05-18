// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "utils.h"

namespace Modules {



void ModuleHelperModule::registerModule() {
    std::vector<FunctParameterInfo> params = {};

    // List all modules
    REGISTER_FUNCTION("module_list", Symbols::Variables::Type::OBJECT, params,
                      "List all available modules with their registered entities",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          if (!args.empty()) {
                              throw std::runtime_error("module_list expects no arguments");
                          }

                          auto &                    umm     = UnifiedModuleManager::instance();
                          auto                      modules = umm.getPluginModules();
                          auto                      paths   = umm.getPluginPaths();
                          Symbols::ObjectMap modulesMap;

                          for (size_t i = 0; i < modules.size(); ++i) {
                              BaseModule * mod  = modules[i];
                              std::string  path = (i < paths.size() ? paths[i] : std::string());
                              modulesMap[std::to_string(i)] =
                                  Symbols::ValuePtr::create(ModuleHelperModule::buildModuleInfoMap(mod, path, umm));
                          }
                          return Symbols::ValuePtr::create(modulesMap);
                      });

    // Check if module exists
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to check" }
    };
    REGISTER_FUNCTION("module_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a module with the given name exists",
                      [](const FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("module_exists expects exactly one string argument");
                          }

                          std::string query   = args[0]->get<std::string>();
                          auto &      umm     = UnifiedModuleManager::instance();
                          auto        modules = umm.getPluginModules();
                          auto        paths   = umm.getPluginPaths();

                          for (size_t i = 0; i < modules.size(); ++i) {
                              BaseModule * mod  = modules[i];
                              std::string  path = (i < paths.size() ? paths[i] : std::string());
                              std::string  name = utils::get_filename_stem(path);
                              if (name.rfind("lib", 0) == 0) {
                                  name = name.substr(3);
                              }
                              if (name == query || mod->name() == query) {
                                  return Symbols::ValuePtr::create(true);
                              }
                          }
                          return Symbols::ValuePtr::create(false);
                      });

    // Get module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to get info for" }
    };
    REGISTER_FUNCTION("module_info", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed information about a specific module including its registered entities",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("module_info expects exactly one string argument");
                          }

                          std::string query   = args[0]->get<std::string>();
                          auto &      umm     = UnifiedModuleManager::instance();
                          auto        modules = umm.getPluginModules();
                          auto        paths   = umm.getPluginPaths();

                          for (size_t i = 0; i < modules.size(); ++i) {
                              BaseModule * mod  = modules[i];
                              std::string  path = (i < paths.size() ? paths[i] : std::string());
                              std::string  name = utils::get_filename_stem(path);
                              if (name.rfind("lib", 0) == 0) {
                                  name = name.substr(3);
                              }
                              if (name == query) {
                                  return Symbols::ValuePtr::create(ModuleHelperModule::buildModuleInfoMap(mod, path, umm));
                              }
                          }
                          return Symbols::ValuePtr::createObjectMap();
                      });

    // Print detailed module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to print info for" }
    };

    REGISTER_FUNCTION("module_print_info", Symbols::Variables::Type::NULL_TYPE, params,
                      "Print detailed information about a module in a formatted way",
                      ModuleHelperModule::ModulePrintInfo);

    // Get function documentation
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the function to get documentation for" }
    };
    REGISTER_FUNCTION("function_doc", Symbols::Variables::Type::OBJECT, params,
                      "Get documentation for a specific function including parameters and return type",
                      ModuleHelperModule::FunctionInfo);
}

}  // namespace Modules
