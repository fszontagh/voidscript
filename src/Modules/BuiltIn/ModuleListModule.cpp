// ModuleListModule.cpp
#include "ModuleListModule.hpp"

#include <filesystem>
#include <string>

#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/ClassRegistry.hpp"

namespace Modules {

void ModuleListModule::registerModule() {
    auto & mgr = ModuleManager::instance();
    mgr.registerFunction("module_list", [](const std::vector<Symbols::Value> & args) -> Symbols::Value {
        if (!args.empty()) {
            throw Exception("module_list expects no arguments");
        }
        using namespace Symbols;
        auto &mm = ModuleManager::instance();
        auto modules = mm.getPluginModules();
        auto paths = mm.getPluginPaths();
        Value::ObjectMap modulesMap;
        for (size_t i = 0; i < modules.size(); ++i) {
            BaseModule *mod = modules[i];
            std::string path = (i < paths.size() ? paths[i] : std::string());
            std::string name = std::filesystem::path(path).stem().string();
            if (name.rfind("lib", 0) == 0) {
                name = name.substr(3);
            }
            // Classes
            Value::ObjectMap classesMap;
            auto &cr = ClassRegistry::instance();
            auto classNames = cr.getClassNames();
            int ci = 0;
            for (const auto &cls : classNames) {
                if (cr.getClassModule(cls) == mod) {
                    classesMap[std::to_string(ci++)] = Value(cls);
                }
            }
            // Functions
            Value::ObjectMap funcsMap;
            auto funcNames = mm.getFunctionNamesForModule(mod);
            int fi = 0;
            for (const auto &fn : funcNames) {
                funcsMap[std::to_string(fi++)] = Value(fn);
            }
            // Variables (not tracked)
            Value::ObjectMap varsMap;
            // Build module info
            Value::ObjectMap infoMap;
            infoMap["name"] = Value(name);
            infoMap["path"] = Value(path);
            infoMap["classes"] = Value(classesMap);
            infoMap["functions"] = Value(funcsMap);
            infoMap["variables"] = Value(varsMap);
            modulesMap[std::to_string(i)] = Value(infoMap);
        }
        return Value(modulesMap);
    });
}

}  // namespace Modules
