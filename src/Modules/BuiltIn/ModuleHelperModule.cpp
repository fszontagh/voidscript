// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

Symbols::Value::ObjectMap ModuleHelperModule::buildModuleInfoMap(BaseModule * module, const std::string & path,
                                                                 const UnifiedModuleManager & umm) {
    Symbols::Value::ObjectMap classesMap;
    auto                      classNames = umm.getClassNames();
    int                       ci         = 0;
    for (const auto & cls : classNames) {
        if (umm.getClassModule(cls) == module) {
            classesMap[std::to_string(ci++)] = Symbols::Value(cls);
        }
    }

    Symbols::Value::ObjectMap funcsMap;
    auto                      funcNames = umm.getFunctionNamesForModule(module);
    int                       fi        = 0;
    for (const auto & fn : funcNames) {
        funcsMap[std::to_string(fi++)] = Symbols::Value(fn);
    }

    Symbols::Value::ObjectMap varsMap;

    Symbols::Value::ObjectMap infoMap;
    infoMap["name"]      = Symbols::Value(std::filesystem::path(path).stem().string());
    infoMap["path"]      = Symbols::Value(path);
    infoMap["classes"]   = Symbols::Value(classesMap);
    infoMap["functions"] = Symbols::Value(funcsMap);
    infoMap["variables"] = Symbols::Value(varsMap);

    return infoMap;
}

void ModuleHelperModule::registerModule(IModuleContext & context) {
    auto &                          umm    = UnifiedModuleManager::instance();
    std::vector<FunctParameterInfo> params = {};
    REGISTER_FUNCTION_WITH_DOC(context, this->name(), "module_list", Symbols::Variables::Type::OBJECT, params,
                               "List all available modules", [](FunctionArguments & args) -> Symbols::Value {
                                   if (!args.empty()) {
                                       throw std::runtime_error("module_list expects no arguments");
                                   }

                                   auto &                    umm     = UnifiedModuleManager::instance();
                                   auto                      modules = umm.getPluginModules();
                                   auto                      paths   = umm.getPluginPaths();
                                   Symbols::Value::ObjectMap modulesMap;
                                   for (size_t i = 0; i < modules.size(); ++i) {
                                       BaseModule * mod  = modules[i];
                                       std::string  path = (i < paths.size() ? paths[i] : std::string());
                                       std::string  name = std::filesystem::path(path).stem().string();
                                       if (name.rfind("lib", 0) == 0) {
                                           name = name.substr(3);
                                       }
                                       // Build info object
                                       Symbols::Value::ObjectMap infoMap =
                                           buildModuleInfoMap(mod, path, umm);
                                       modulesMap[std::to_string(i)] = Symbols::Value(infoMap);
                                   }
                                   return Symbols::Value(modulesMap);
                               });

    params = {
        { "name", Symbols::Variables::Type::STRING }
    };
    REGISTER_FUNCTION_WITH_DOC(context, this->name(), "module_exists", Symbols::Variables::Type::BOOLEAN, params, "",
                               [](FunctionArguments & args) -> Symbols::Value {
                                   using namespace Symbols;
                                   if (args.size() != 1 || args[0].getType() != Variables::Type::STRING) {
                                       throw std::runtime_error("module_exists expects exactly one string argument");
                                   }
                                   std::string query = Value::to_string(args[0].get());
                                   auto &      umm   = UnifiedModuleManager::instance();
                                   auto        paths = umm.getPluginPaths();
                                   for (const auto & path : paths) {
                                       std::string name = std::filesystem::path(path).stem().string();
                                       if (name.rfind("lib", 0) == 0) {
                                           name = name.substr(3);
                                       }
                                       if (name == query) {
                                           return Value(true);
                                       }
                                   }
                                   return Value(false);
                               });

    REGISTER_FUNCTION_WITH_DOC(
        context, this->name(), "module_info", Symbols::Variables::Type::OBJECT,
        std::vector<FunctParameterInfo>{
            { "name", Symbols::Variables::Type::STRING }
        }, "Retrieve information about a specific module",
        [](FunctionArguments & args) -> Symbols::Value {
            using namespace Symbols;
            if (args.size() != 1 || args[0].getType() != Variables::Type::STRING) {
                throw std::runtime_error("module_info expects exactly one string argument");
            }
            std::string query   = Value::to_string(args[0].get());
            auto &      umm     = UnifiedModuleManager::instance();
            auto        modules = umm.getPluginModules();
            auto        paths   = umm.getPluginPaths();
            for (size_t i = 0; i < modules.size(); ++i) {
                BaseModule * mod  = modules[i];
                std::string  path = (i < paths.size() ? paths[i] : std::string());
                std::string  name = std::filesystem::path(path).stem().string();
                if (name.rfind("lib", 0) == 0) {
                    name = name.substr(3);
                }
                if (name == query) {
                    return Symbols::Value(buildModuleInfoMap(mod, path, umm));
                }
            }
            return Value::ObjectMap{};
        });
}

}  // namespace Modules
