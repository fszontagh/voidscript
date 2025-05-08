// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

Symbols::Value::ObjectMap ModuleHelperModule::buildModuleInfoMap(BaseModule* module, const std::string& path,
                                                               const UnifiedModuleManager& umm) {
    Symbols::Value::ObjectMap classesMap;
    auto classNames = umm.getClassNames();
    int ci = 0;
    for (const auto& cls : classNames) {
        if (umm.getClassModule(cls) == module) {
            classesMap[std::to_string(ci++)] = buildClassInfoMap(cls, umm);
        }
    }

    Symbols::Value::ObjectMap funcsMap;
    auto funcNames = umm.getFunctionNamesForModule(module);
    int fi = 0;
    for (const auto& fn : funcNames) {
        funcsMap[std::to_string(fi++)] = buildFunctionInfoMap(fn, umm);
    }

    Symbols::Value::ObjectMap varsMap;

    Symbols::Value::ObjectMap infoMap;
    infoMap["name"] = Symbols::Value(std::filesystem::path(path).stem().string());
    infoMap["path"] = Symbols::Value(path);
    infoMap["classes"] = Symbols::Value(classesMap);
    infoMap["functions"] = Symbols::Value(funcsMap);
    infoMap["variables"] = Symbols::Value(varsMap);
    infoMap["documentation"] = Symbols::Value::ObjectMap(); // Placeholder for future module-level docs

    return infoMap;
}

Symbols::Value::ObjectMap ModuleHelperModule::buildClassInfoMap(const std::string& className,
                                                              const UnifiedModuleManager& umm) {
    Symbols::Value::ObjectMap classInfo;
    classInfo["name"] = Symbols::Value(className);
    
    // Get methods for this class - using available methods
    Symbols::Value::ObjectMap methodsMap;
    auto methods = umm.getFunctionNamesForModule(umm.getClassModule(className));
    int mi = 0;
    for (const auto& method : methods) {
        // Only include methods that belong to this class
        if (method.find(className + "::") == 0) {
            methodsMap[std::to_string(mi++)] = buildMethodInfoMap(className, method, umm);
        }
    }
    classInfo["methods"] = Symbols::Value(methodsMap);
    
    return classInfo;
}

Symbols::Value::ObjectMap ModuleHelperModule::buildFunctionInfoMap(const std::string& functionName,
                                                                 const UnifiedModuleManager& umm) {
    Symbols::Value::ObjectMap funcInfo;
    funcInfo["name"] = Symbols::Value(functionName);
    
    // Get function documentation
    auto docInfo = buildFunctionDocMap(functionName, umm);
    funcInfo["documentation"] = Symbols::Value(docInfo);
    
    return funcInfo;
}

Symbols::Value::ObjectMap ModuleHelperModule::buildMethodInfoMap(const std::string& className,
                                                               const std::string& methodName,
                                                               const UnifiedModuleManager& umm) {
    Symbols::Value::ObjectMap methodInfo;
    methodInfo["name"] = Symbols::Value(methodName);
    methodInfo["class"] = Symbols::Value(className);
    
    // Get method documentation
    auto docInfo = buildFunctionDocMap(methodName, umm);
    methodInfo["documentation"] = Symbols::Value(docInfo);
    
    return methodInfo;
}

Symbols::Value::ObjectMap ModuleHelperModule::buildParameterInfoMap(const FunctParameterInfo& param) {
    Symbols::Value::ObjectMap paramInfo;
    paramInfo["name"] = Symbols::Value(param.name);
    paramInfo["type"] = Symbols::Value(Symbols::Variables::TypeToString(param.type));
    paramInfo["description"] = Symbols::Value(param.description);
    paramInfo["optional"] = Symbols::Value(param.optional);
    paramInfo["interpolate"] = Symbols::Value(param.interpolate);
    return paramInfo;
}

Symbols::Value::ObjectMap ModuleHelperModule::buildFunctionDocMap(const std::string& functionName,
                                                                const UnifiedModuleManager& umm) {
    Symbols::Value::ObjectMap docInfo;
    
    // Get function info from UMM - using available methods
    auto modules = umm.getPluginModules();
    for (auto* module : modules) {
        auto funcNames = umm.getFunctionNamesForModule(module);
        for (const auto& fn : funcNames) {
            if (fn == functionName) {
                // We found the function, but we don't have direct access to its info
                // So we'll provide basic documentation structure
                docInfo["name"] = Symbols::Value(functionName);
                docInfo["description"] = Symbols::Value("Function documentation not available");
                docInfo["return_type"] = Symbols::Value("unknown");
                docInfo["parameters"] = Symbols::Value::ObjectMap();
                return docInfo;
            }
        }
    }
    
    return docInfo;
}

void ModuleHelperModule::registerModule() {
    std::vector<FunctParameterInfo> params = {};
    
    // List all modules
    REGISTER_FUNCTION("module_list", Symbols::Variables::Type::OBJECT, params, 
        "List all available modules with their registered entities",
        [this](const FunctionArguments& args) -> Symbols::Value {
            if (!args.empty()) {
                throw std::runtime_error("module_list expects no arguments");
            }

            auto& umm = UnifiedModuleManager::instance();
            auto modules = umm.getPluginModules();
            auto paths = umm.getPluginPaths();
            Symbols::Value::ObjectMap modulesMap;
            
            for (size_t i = 0; i < modules.size(); ++i) {
                BaseModule* mod = modules[i];
                std::string path = (i < paths.size() ? paths[i] : std::string());
                modulesMap[std::to_string(i)] = Symbols::Value(buildModuleInfoMap(mod, path, umm));
            }
            return Symbols::Value(modulesMap);
        });

    // Check if module exists
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to check" }
    };
    REGISTER_FUNCTION("module_exists", Symbols::Variables::Type::BOOLEAN, params,
        "Check if a module with the given name exists",
        [this](const FunctionArguments& args) -> Symbols::Value {
            if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("module_exists expects exactly one string argument");
            }
            
            std::string query = Symbols::Value::to_string(args[0].get());
            auto& umm = UnifiedModuleManager::instance();
            auto modules = umm.getPluginModules();
            auto paths = umm.getPluginPaths();
            
            for (size_t i = 0; i < modules.size(); ++i) {
                BaseModule* mod = modules[i];
                std::string path = (i < paths.size() ? paths[i] : std::string());
                std::string name = std::filesystem::path(path).stem().string();
                if (name.rfind("lib", 0) == 0) {
                    name = name.substr(3);
                }
                if (name == query || mod->name() == query) {
                    return Symbols::Value(true);
                }
            }
            return Symbols::Value(false);
        });

    // Get module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to get info for" }
    };
    REGISTER_FUNCTION("module_info", Symbols::Variables::Type::OBJECT, params,
        "Get detailed information about a specific module including its registered entities",
        [this](const FunctionArguments& args) -> Symbols::Value {
            if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("module_info expects exactly one string argument");
            }
            
            std::string query = Symbols::Value::to_string(args[0].get());
            auto& umm = UnifiedModuleManager::instance();
            auto modules = umm.getPluginModules();
            auto paths = umm.getPluginPaths();
            
            for (size_t i = 0; i < modules.size(); ++i) {
                BaseModule* mod = modules[i];
                std::string path = (i < paths.size() ? paths[i] : std::string());
                std::string name = std::filesystem::path(path).stem().string();
                if (name.rfind("lib", 0) == 0) {
                    name = name.substr(3);
                }
                if (name == query) {
                    return Symbols::Value(buildModuleInfoMap(mod, path, umm));
                }
            }
            return Symbols::Value::ObjectMap{};
        });

    // Print detailed module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to print info for" }
    };
    REGISTER_FUNCTION("module_print_info", Symbols::Variables::Type::NULL_TYPE, params,
        "Print detailed information about a module in a formatted way",
        [this](const FunctionArguments& args) -> Symbols::Value {
            if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("module_print_info expects exactly one string argument");
            }
            
            std::string query = Symbols::Value::to_string(args[0].get());
            auto& umm = UnifiedModuleManager::instance();
            auto modules = umm.getPluginModules();
            auto paths = umm.getPluginPaths();
            
            for (size_t i = 0; i < modules.size(); ++i) {
                BaseModule* mod = modules[i];
                std::string path = (i < paths.size() ? paths[i] : std::string());
                std::string name = std::filesystem::path(path).stem().string();
                if (name.rfind("lib", 0) == 0) {
                    name = name.substr(3);
                }
                if (name == query) {
                    // Get module info
                    auto infoMap = buildModuleInfoMap(mod, path, umm);
                    
                    // Print module header
                    std::cout << "Module name: " << name << "\n";
                    std::cout << "Path: " << path << "\n\n";
                    
                    // Print classes
                    auto classesMap = std::get<Symbols::Value::ObjectMap>(infoMap["classes"].get());
                    if (!classesMap.empty()) {
                        std::cout << "Classes:\n";
                        for (const auto& [_, classInfo] : classesMap) {
                            auto classMap = std::get<Symbols::Value::ObjectMap>(classInfo.get());
                            std::cout << "- " << std::get<std::string>(classMap["name"].get()) << "\n";
                            
                            // Print methods
                            auto methodsMap = std::get<Symbols::Value::ObjectMap>(classMap["methods"].get());
                            for (const auto& [_, methodInfo] : methodsMap) {
                                auto methodMap = std::get<Symbols::Value::ObjectMap>(methodInfo.get());
                                std::cout << "  - " << std::get<std::string>(methodMap["name"].get());
                                
                                // Print method documentation if available
                                auto docMap = std::get<Symbols::Value::ObjectMap>(methodMap["documentation"].get());
                                if (!docMap.empty()) {
                                    if (docMap.contains("return_type")) {
                                        std::cout << " -> " << std::get<std::string>(docMap["return_type"].get());
                                    }
                                    std::cout << "\n";
                                    
                                    // Print parameters
                                    if (docMap.contains("parameters")) {
                                        auto paramsMap = std::get<Symbols::Value::ObjectMap>(docMap["parameters"].get());
                                        for (const auto& [_, paramInfo] : paramsMap) {
                                            auto paramMap = std::get<Symbols::Value::ObjectMap>(paramInfo.get());
                                            std::cout << "    - " << std::get<std::string>(paramMap["name"].get());
                                            std::cout << " type: " << std::get<std::string>(paramMap["type"].get());
                                            if (std::get<bool>(paramMap["optional"].get())) {
                                                std::cout << " (optional)";
                                            }
                                            if (std::get<bool>(paramMap["interpolate"].get())) {
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
                    auto funcsMap = std::get<Symbols::Value::ObjectMap>(infoMap["functions"].get());
                    if (!funcsMap.empty()) {
                        std::cout << "Functions:\n";
                        for (const auto& [_, funcInfo] : funcsMap) {
                            auto funcMap = std::get<Symbols::Value::ObjectMap>(funcInfo.get());
                            std::cout << "- " << std::get<std::string>(funcMap["name"].get());
                            
                            // Print function documentation if available
                            auto docMap = std::get<Symbols::Value::ObjectMap>(funcMap["documentation"].get());
                            if (!docMap.empty()) {
                                if (docMap.contains("return_type")) {
                                    std::cout << " -> " << std::get<std::string>(docMap["return_type"].get());
                                }
                                std::cout << "\n";
                                
                                // Print parameters
                                if (docMap.contains("parameters")) {
                                    auto paramsMap = std::get<Symbols::Value::ObjectMap>(docMap["parameters"].get());
                                    for (const auto& [_, paramInfo] : paramsMap) {
                                        auto paramMap = std::get<Symbols::Value::ObjectMap>(paramInfo.get());
                                        std::cout << "  - " << std::get<std::string>(paramMap["name"].get());
                                        std::cout << " type: " << std::get<std::string>(paramMap["type"].get());
                                        if (std::get<bool>(paramMap["optional"].get())) {
                                            std::cout << " (optional)";
                                        }
                                        if (std::get<bool>(paramMap["interpolate"].get())) {
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
                    
                    return Symbols::Value();
                }
            }
            
            std::cout << "Module not found: " << query << "\n";
            return Symbols::Value();
        });

    // Get function documentation
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the function to get documentation for" }
    };
    REGISTER_FUNCTION("function_doc", Symbols::Variables::Type::OBJECT, params,
        "Get documentation for a specific function including parameters and return type",
        [this](const FunctionArguments& args) -> Symbols::Value {
            if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("function_doc expects exactly one string argument");
            }
            
            std::string functionName = Symbols::Value::to_string(args[0].get());
            auto& umm = UnifiedModuleManager::instance();
            return Symbols::Value(buildFunctionDocMap(functionName, umm));
        });
}

}  // namespace Modules
