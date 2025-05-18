// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "utils.h"

namespace Modules {

/**
 * @brief Module providing helper functions for module information and documentation.
 */
class ModuleHelperModule : public BaseModule {
  public:
    ModuleHelperModule() { setModuleName("ModuleHelper"); }

    void registerModule() override;

  private:
    // Helper methods for building module information
    static Symbols::Value::ObjectMap buildModuleInfoMap(BaseModule * module, const std::string & path,
                                                        const UnifiedModuleManager & umm) {
        Symbols::Value::ObjectMap classesMap;
        auto                      classNames = umm.getClassNames();

        Symbols::Value::ObjectMap funcsMap;
        auto                      funcNames = umm.getFunctionNamesForModule(module);

        int ci = 0;
        for (const auto & cls : classNames) {
            if (umm.getClassModule(cls) == module) {
                classesMap[std::to_string(ci++)] = Symbols::Value::create(buildClassInfoMap(cls, umm));
            }
        }

        int fi = 0;
        for (const auto & fn : funcNames) {
            funcsMap[std::to_string(fi++)] = Symbols::Value::create(buildFunctionInfoMap(fn, umm));
        }

        Symbols::Value::ObjectMap varsMap;

        Symbols::Value::ObjectMap infoMap;
        infoMap["name"]          = Symbols::Value::create(utils::get_filename_stem(path));
        infoMap["path"]          = Symbols::Value::create(path);
        infoMap["classes"]       = Symbols::Value::create(classesMap);
        infoMap["functions"]     = Symbols::Value::create(funcsMap);
        infoMap["variables"]     = Symbols::Value::create(varsMap);
        infoMap["documentation"] = Symbols::Value::CreateObjectMap();

        return infoMap;
    }

    // Helper methods for building entity information
    static Symbols::Value::ObjectMap buildClassInfoMap(const std::string &          className,
                                                       const UnifiedModuleManager & umm) {
        Symbols::Value::ObjectMap classInfo;
        classInfo["name"] = Symbols::Value::create(className);

        Symbols::Value::ObjectMap methodsMap;
        auto                      methods = umm.getFunctionNamesForModule(umm.getClassModule(className));
        int                       mi      = 0;
        for (const auto & method : methods) {
            if (method.find(className + Symbols::SymbolContainer::SCOPE_SEPARATOR) == 0) {
                methodsMap[std::to_string(mi++)] = Symbols::Value::create(buildMethodInfoMap(className, method, umm));
            }
        }
        classInfo["methods"] = Symbols::Value::create(methodsMap);

        return classInfo;
    };

    static Symbols::Value::ObjectMap buildFunctionInfoMap(const std::string &          functionName,
                                                          const UnifiedModuleManager & umm) {
        Symbols::Value::ObjectMap funcInfo;
        funcInfo["name"] = Symbols::Value::create(functionName);

        // Get function documentation
        auto docInfo              = buildFunctionDocMap(functionName, umm);
        funcInfo["documentation"] = Symbols::Value::create(docInfo);

        return funcInfo;
    }

    static Symbols::Value::ObjectMap buildMethodInfoMap(const std::string & className, const std::string & methodName,
                                                        const UnifiedModuleManager & umm) {
        Symbols::Value::ObjectMap methodInfo;
        methodInfo["name"]          = Symbols::Value::create(methodName);
        methodInfo["class"]         = Symbols::Value::create(className);
        auto docInfo                = ModuleHelperModule::buildFunctionDocMap(methodName, umm);
        methodInfo["documentation"] = Symbols::Value::create(docInfo);
        return methodInfo;
    }

    // Helper methods for building documentation
    static Symbols::Value::ObjectMap buildParameterInfoMap(const FunctParameterInfo & param) {
        Symbols::Value::ObjectMap paramInfo;
        paramInfo["name"]        = Symbols::Value::create(param.name);
        paramInfo["type"]        = Symbols::Value::create(Symbols::Variables::TypeToString(param.type));
        paramInfo["description"] = Symbols::Value::create(param.description);
        paramInfo["optional"]    = Symbols::Value::create(param.optional);
        paramInfo["interpolate"] = Symbols::Value::create(param.interpolate);
        return paramInfo;
    }

    static Symbols::Value::ObjectMap buildFunctionDocMap(const std::string &          functionName,
                                                         const UnifiedModuleManager & umm) {
        Symbols::Value::ObjectMap docInfo;
        auto                      modules = umm.getPluginModules();
        for (auto * module : modules) {
            auto funcNames = umm.getFunctionNamesForModule(module);
            for (const auto & fn : funcNames) {
                if (fn == functionName) {
                    auto funcDoc           = umm.getFunctionDoc(fn);
                    docInfo["name"]        = Symbols::Value::create(funcDoc.name);
                    docInfo["description"] = Symbols::Value::create(funcDoc.description);
                    docInfo["return_type"] =
                        Symbols::Value::create(Symbols::Variables::TypeToString(funcDoc.returnType));
                    Symbols::Value::ObjectMap paramsMap;
                    for (const auto & param : funcDoc.parameterList) {
                        paramsMap[param.name] = Symbols::Value::create(buildParameterInfoMap(param));
                    }
                    docInfo["parameters"] = Symbols::Value::create(paramsMap);
                    return docInfo;
                }
            }
        }
        return docInfo;
    }

    static Symbols::Value::ValuePtr FunctionInfo(const FunctionArguments & args) {
        if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("function_doc expects exactly one string argument");
        }

        std::string functionName = *args[0];
        auto &      umm          = UnifiedModuleManager::instance();
        return Symbols::Value::create(buildFunctionDocMap(functionName, umm));
    };

    static Symbols::Value::ValuePtr ModulePrintInfo(const FunctionArguments & args) {
        if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("module_print_info expects exactly one string argument");
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
                // Get module info
                auto infoMap = ModuleHelperModule::buildModuleInfoMap(mod, path, umm);

                // Print module header
                std::cout << "Module name: " << name << "\n";
                std::cout << "Path: " << path << "\n\n";

                // Print classes
                auto classesMap = infoMap["classes"]->get<Symbols::Value::ObjectMap>();
                if (!classesMap.empty()) {
                    std::cout << "Classes:\n";
                    for (const auto & [_, classInfo] : classesMap) {
                        auto classMap = classInfo->get<Symbols::Value::ObjectMap>();
                        std::cout << "- " << classMap["name"]->get<std::string>() << "\n";

                        // Print methods
                        auto methodsMap = classMap["methods"]->get<Symbols::Value::ObjectMap>();
                        for (const auto & [_, methodInfo] : methodsMap) {
                            auto methodMap = methodInfo->get<Symbols::Value::ObjectMap>();
                            std::cout << "  - " << methodMap["name"]->get<std::string>();
                            // Print method documentation if available
                            auto docMap = methodMap["documentation"]->get<Symbols::Value::ObjectMap>();
                            if (!docMap.empty()) {
                                if (docMap.contains("return_type")) {
                                    std::cout << " -> " << docMap["return_type"]->get<std::string>();
                                }
                                std::cout << "\n";

                                // Print parameters
                                if (docMap.contains("parameters")) {
                                    auto paramsMap = docMap["parameters"]->get<Symbols::Value::ObjectMap>();
                                    for (const auto & [_, paramInfo] : paramsMap) {
                                        auto paramMap = paramInfo->get<Symbols::Value::ObjectMap>();
                                        std::cout << "    - " << paramMap["name"]->get<std::string>();
                                        std::cout << " type: " << paramMap["type"]->get<std::string>();
                                        if (paramMap["optional"]->get<bool>()) {
                                            std::cout << " (optional)";
                                        }
                                        if (paramMap["interpolate"]->get<bool>()) {
                                            std::cout << " (interpolate)";
                                        }
                                        std::cout << "\n";
                                    }
                                }
                            } else {
                                std::cout << "\n";
                            }
                        }
                    }
                    std::cout << "\n";
                }

                // Print functions
                auto funcsMap = infoMap["functions"]->get<Symbols::Value::ObjectMap>();
                if (!funcsMap.empty()) {
                    std::cout << "Functions:\n";
                    for (const auto & [_, funcInfo] : funcsMap) {
                        auto funcMap = funcInfo->get<Symbols::Value::ObjectMap>();
                        std::cout << "- " << funcMap["name"]->get<std::string>();

                        // Print function documentation if available
                        Symbols::Value::ObjectMap docMap = *funcMap["documentation"];
                        if (!docMap.empty()) {
                            if (docMap.contains("return_type")) {
                                std::cout << " -> " << docMap["return_type"]->get<std::string>();
                            }
                            std::cout << "\n";

                            // Print parameters
                            if (docMap.contains("parameters")) {
                                Symbols::Value::ObjectMap paramsMap = *docMap["parameters"];
                                for (const auto & [_, paramInfo] : paramsMap) {
                                    auto paramMap = paramInfo->get<Symbols::Value::ObjectMap>();
                                    std::cout << "  - " << paramMap["name"]->get<std::string>();
                                    std::cout << " type: " << paramMap["type"]->get<std::string>();
                                    if (paramMap["optional"]->get<bool>()) {
                                        std::cout << " (optional)";
                                    }
                                    if (paramMap["interpolate"]->get<bool>()) {
                                        std::cout << " (interpolate)";
                                    }
                                    std::cout << "\n";
                                }
                            }
                        } else {
                            std::cout << "\n";
                        }
                    }
                }

                return nullptr;
            }
        }

        std::cout << "Module not found: " << query << "\n";
        return nullptr;
    };
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
