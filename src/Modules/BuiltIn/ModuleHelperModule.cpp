// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/ModuleManager.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

void ModuleHelperModule::registerModule() {
    auto & mgr = ModuleManager::instance();
    // List all loaded plugin modules
    mgr.registerFunction("module_list", [](FunctionArguments & args) -> Symbols::Value {
        if (!args.empty()) {
            throw std::runtime_error("module_list expects no arguments");
        }

        auto &                    mm      = ModuleManager::instance();
        auto                      modules = mm.getPluginModules();
        auto                      paths   = mm.getPluginPaths();
        Symbols::Value::ObjectMap modulesMap;
        for (size_t i = 0; i < modules.size(); ++i) {
            BaseModule * mod  = modules[i];
            std::string  path = (i < paths.size() ? paths[i] : std::string());
            std::string  name = std::filesystem::path(path).stem().string();
            if (name.rfind("lib", 0) == 0) {
                name = name.substr(3);
            }
            // Collect classes
            Symbols::Value::ObjectMap classesMap;
            auto &                    cr         = Symbols::ClassRegistry::instance();
            auto                      classNames = cr.getClassNames();
            int                       ci         = 0;
            for (const auto & cls : classNames) {
                if (cr.getClassModule(cls) == mod) {
                    classesMap[std::to_string(ci++)] = Symbols::Value(cls);
                }
            }
            // Collect functions
            Symbols::Value::ObjectMap funcsMap;
            auto                      funcNames = mm.getFunctionNamesForModule(mod);
            int                       fi        = 0;
            for (const auto & fn : funcNames) {
                funcsMap[std::to_string(fi++)] = Symbols::Value(fn);
            }
            // Variables (not tracked)
            Symbols::Value::ObjectMap varsMap;
            // Build info object
            Symbols::Value::ObjectMap infoMap;
            infoMap["name"]               = Symbols::Value(name);
            infoMap["path"]               = Symbols::Value(path);
            infoMap["classes"]            = Symbols::Value(classesMap);
            infoMap["functions"]          = Symbols::Value(funcsMap);
            infoMap["variables"]          = Symbols::Value(varsMap);
            modulesMap[std::to_string(i)] = Symbols::Value(infoMap);
        }
        return Symbols::Value(modulesMap);
    });

    // Check if a module exists by name
    mgr.registerFunction("module_exists", [](FunctionArguments & args) -> Symbols::Value {
        using namespace Symbols;
        if (args.size() != 1 || args[0].getType() != Variables::Type::STRING) {
            throw std::runtime_error("module_exists expects exactly one string argument");
        }
        std::string query = Value::to_string(args[0].get());
        auto &      mm    = ModuleManager::instance();
        auto        paths = mm.getPluginPaths();
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

    // Get info for a specific module
    mgr.registerFunction("module_info", [](FunctionArguments & args) -> Symbols::Value {
        using namespace Symbols;
        if (args.size() != 1 || args[0].getType() != Variables::Type::STRING) {
            throw std::runtime_error("module_info expects exactly one string argument");
        }
        std::string query   = Value::to_string(args[0].get());
        auto &      mm      = ModuleManager::instance();
        auto        modules = mm.getPluginModules();
        auto        paths   = mm.getPluginPaths();
        for (size_t i = 0; i < modules.size(); ++i) {
            BaseModule * mod  = modules[i];
            std::string  path = (i < paths.size() ? paths[i] : std::string());
            std::string  name = std::filesystem::path(path).stem().string();
            if (name.rfind("lib", 0) == 0) {
                name = name.substr(3);
            }
            if (name == query) {
                // Build same infoMap as module_list
                Value::ObjectMap classesMap;
                auto &           cr         = ClassRegistry::instance();
                auto             classNames = cr.getClassNames();
                int              ci         = 0;
                for (const auto & cls : classNames) {
                    if (cr.getClassModule(cls) == mod) {
                        classesMap[std::to_string(ci++)] = Value(cls);
                    }
                }
                Value::ObjectMap funcsMap;
                auto             funcNames = mm.getFunctionNamesForModule(mod);
                int              fi        = 0;
                for (const auto & fn : funcNames) {
                    funcsMap[std::to_string(fi++)] = Value(fn);
                }
                Value::ObjectMap varsMap;
                Value::ObjectMap infoMap;
                infoMap["name"]      = Value(name);
                infoMap["path"]      = Value(path);
                infoMap["classes"]   = Value(classesMap);
                infoMap["functions"] = Value(funcsMap);
                infoMap["variables"] = Value(varsMap);
                return Value(infoMap);
            }
        }
        return Value::ObjectMap{};
    });
}

}  // namespace Modules
