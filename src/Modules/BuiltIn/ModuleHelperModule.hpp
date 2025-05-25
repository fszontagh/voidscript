// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/FunctionParameterInfo.hpp"
#include "utils.h"

namespace Modules {

/**
 * @brief Module providing helper functions for module information and documentation.
 */
class ModuleHelperModule : public BaseModule {
  public:
    ModuleHelperModule() { setModuleName("ModuleHelper"); }

    void registerFunctions() override;

  private:
    // Helper methods for building module information
    static Symbols::ObjectMap buildModuleInfoMap(BaseModule * module, const std::string & path,
                                                 const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap classesMap;
        auto               classNames = sc->getClassNames();

        Symbols::ObjectMap funcsMap;
        auto               funcNames = sc->getFunctionNames();

        int ci = 0;
        for (const auto & cls : classNames) {
            // We don't have module-specific class filtering anymore, so we include all classes
            classesMap[std::to_string(ci++)] = Symbols::ValuePtr(buildClassInfoMap(cls, sc));
        }

        int fi = 0;
        for (const auto & fn : funcNames) {
            funcsMap[std::to_string(fi++)] = Symbols::ValuePtr(buildFunctionInfoMap(fn, sc));
        }

        Symbols::ObjectMap varsMap;

        Symbols::ObjectMap infoMap;
        infoMap["name"]          = utils::get_filename_stem(path);  //Symbols::ValuePtr(utils::get_filename_stem(path));
        infoMap["path"]          = path;                            //Symbols::ValuePtr(path);
        infoMap["classes"]       = classesMap;                      //Symbols::ValuePtr(classesMap);
        infoMap["functions"]     = funcsMap;                        //Symbols::ValuePtr(funcsMap);
        infoMap["variables"]     = varsMap;                         //Symbols::ValuePtr(varsMap);
        infoMap["documentation"] = Symbols::ObjectMap();

        return infoMap;
    }

    // Helper methods for building entity information
    static Symbols::ObjectMap buildClassInfoMap(const std::string & className, const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap classInfo;
        classInfo["name"] = Symbols::ValuePtr(className);

        Symbols::ObjectMap methodsMap;
        auto methodNames = sc->getMethodNames(className);
        int  mi = 0;
        for (const auto & methodName : methodNames) {
            methodsMap[std::to_string(mi++)] = Symbols::ValuePtr(buildMethodInfoMap(className, methodName, sc));
        }
        classInfo["methods"] = Symbols::ValuePtr(methodsMap);

        return classInfo;
    };

    static Symbols::ObjectMap buildFunctionInfoMap(const std::string & functionName, const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap funcInfo;
        funcInfo["name"] = Symbols::ValuePtr(functionName);

        // Get function documentation
        auto docInfo              = buildFunctionDocMap(functionName, sc);
        funcInfo["documentation"] = Symbols::ValuePtr(docInfo);

        return funcInfo;
    }

    static Symbols::ObjectMap buildMethodInfoMap(const std::string & className, const std::string & methodName,
                                                 const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap methodInfo;
        methodInfo["name"]          = Symbols::ValuePtr(methodName);
        methodInfo["class"]         = Symbols::ValuePtr(className);
        
        // Try to get method information from SymbolContainer
        Symbols::ObjectMap docInfo;
        std::string qualifiedMethodName = className + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName;
        
        // First, try to get method documentation if it exists
        auto docInfoFromFunc = ModuleHelperModule::buildFunctionDocMap(qualifiedMethodName, sc);
        if (!docInfoFromFunc.empty()) {
            docInfo = docInfoFromFunc;
        } else {
            // Build documentation from available method information
            docInfo["name"] = Symbols::ValuePtr(qualifiedMethodName);
            
            // Try to get return type from SymbolContainer
            try {
                auto returnType = sc->getMethodReturnType(className, methodName);
                if (returnType != Symbols::Variables::Type::NULL_TYPE) {
                    docInfo["return_type"] = Symbols::ValuePtr(Symbols::Variables::TypeToString(returnType));
                }
                
                // Try to get parameters from SymbolContainer
                auto parameters = sc->getMethodParameters(className, methodName);
                if (!parameters.empty()) {
                    Symbols::ObjectMap paramsMap;
                    for (const auto & param : parameters) {
                        paramsMap[param.name] = Symbols::ValuePtr(buildParameterInfoMap(param));
                    }
                    docInfo["parameters"] = Symbols::ValuePtr(paramsMap);
                }
            } catch (...) {
                // Method not found, continue with minimal info
            }
            
            // Set empty description if not available
            if (!docInfo.contains("description")) {
                docInfo["description"] = Symbols::ValuePtr("");
            }
        }
        
        methodInfo["documentation"] = Symbols::ValuePtr(docInfo);
        return methodInfo;
    }

    // Helper methods for building documentation
    static Symbols::ObjectMap buildParameterInfoMap(const Symbols::FunctionParameterInfo & param) {
        Symbols::ObjectMap paramInfo;
        paramInfo["name"]        = Symbols::ValuePtr(param.name);
        paramInfo["type"]        = Symbols::ValuePtr(Symbols::Variables::TypeToString(param.type));
        paramInfo["description"] = Symbols::ValuePtr(param.description);
        paramInfo["optional"]    = Symbols::ValuePtr(param.optional);
        paramInfo["interpolate"] = Symbols::ValuePtr(param.interpolate);
        return paramInfo;
    }

    static Symbols::ObjectMap buildFunctionDocMap(const std::string & functionName, const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap docInfo;
        
        // First try to find it in the functions
        if (sc->hasFunction(functionName)) {
            auto funcDoc = sc->getFunctionDoc(functionName);
            docInfo["name"] = Symbols::ValuePtr(funcDoc.name);
            docInfo["description"] = Symbols::ValuePtr(funcDoc.description);
            docInfo["return_type"] = Symbols::ValuePtr(Symbols::Variables::TypeToString(funcDoc.returnType));
            Symbols::ObjectMap paramsMap;
            for (const auto & param : funcDoc.parameterList) {
                paramsMap[param.name] = Symbols::ValuePtr(buildParameterInfoMap(param));
            }
            docInfo["parameters"] = Symbols::ValuePtr(paramsMap);
            return docInfo;
        }
        
        // If not found in functions, try to find it in methods (for qualified method names)
        // Parse qualified method name (className::methodName)
        size_t scopePos = functionName.find("::");
        if (scopePos != std::string::npos) {
            std::string className = functionName.substr(0, scopePos);
            std::string methodName = functionName.substr(scopePos + 2);
            
            try {
                if (sc->hasClass(className) && sc->hasMethod(className, methodName)) {
                    auto returnType = sc->getMethodReturnType(className, methodName);
                    auto parameters = sc->getMethodParameters(className, methodName);
                    
                    docInfo["name"] = Symbols::ValuePtr(functionName);
                    docInfo["description"] = Symbols::ValuePtr(""); // No description available
                    docInfo["return_type"] = Symbols::ValuePtr(Symbols::Variables::TypeToString(returnType));
                    
                    Symbols::ObjectMap paramsMap;
                    for (const auto & param : parameters) {
                        paramsMap[param.name] = Symbols::ValuePtr(buildParameterInfoMap(param));
                    }
                    docInfo["parameters"] = Symbols::ValuePtr(paramsMap);
                    return docInfo;
                }
            } catch (...) {
                // Method not found
            }
        }
        
        return docInfo;
    }

    static Symbols::ValuePtr FunctionInfo(const FunctionArguments & args) {
        if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("function_doc expects exactly one string argument");
        }

        const std::string functionName = args[0];
        auto *            sc = Symbols::SymbolContainer::instance();
        return Symbols::ValuePtr(buildFunctionDocMap(functionName, sc));
    };

    static Symbols::ValuePtr ModulePrintInfo(const FunctionArguments & args) {
        if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("module_print_info expects exactly one string argument");
        }

        std::string query = args[0];
        auto *      sc = Symbols::SymbolContainer::instance();
        
        // Since we don't have module-specific information anymore, we'll just print all symbols
        std::string path = "/modules/" + query; // Simulate a path
        
        // Get module info (this will include all classes and functions)
        auto infoMap = ModuleHelperModule::buildModuleInfoMap(nullptr, path, sc);

        // Print module header
        std::cout << "=== Module Information ===\n";
        std::cout << "Module name: " << query << "\n";
        std::cout << "Path: " << path << "\n\n";

        // Print functions (standalone functions, not methods)
        Symbols::ObjectMap funcsMap = infoMap["functions"];
        if (!funcsMap.empty()) {
            std::cout << "=== Standalone Functions ===\n";
            for (const auto & [_, funcInfo] : funcsMap) {
                Symbols::ObjectMap funcMap = funcInfo;
                std::cout << "- " << funcMap["name"]->get<std::string>();

                // Print function documentation if available
                Symbols::ObjectMap docMap = funcMap["documentation"];
                if (!docMap.empty()) {
                    if (docMap.contains("return_type")) {
                        std::cout << " -> " << docMap["return_type"]->get<std::string>();
                    }
                    if (docMap.contains("description") && !docMap["description"]->get<std::string>().empty()) {
                        std::cout << " : " << docMap["description"]->get<std::string>();
                    }
                    std::cout << "\n";

                    // Print parameters
                    if (docMap.contains("parameters")) {
                        Symbols::ObjectMap paramsMap = docMap["parameters"];
                        if (!paramsMap.empty()) {
                            std::cout << "    Parameters:\n";
                            for (const auto & [_, paramInfo] : paramsMap) {
                                Symbols::ObjectMap paramMap = paramInfo;
                                std::cout << "      - " << paramMap["name"]->get<std::string>();
                                std::cout << " (" << paramMap["type"]->get<std::string>() << ")";
                                if (paramMap["optional"]->get<bool>()) {
                                    std::cout << " [optional]";
                                }
                                if (paramMap["interpolate"]->get<bool>()) {
                                    std::cout << " [interpolate]";
                                }
                                if (paramMap.contains("description") && !paramMap["description"]->get<std::string>().empty()) {
                                    std::cout << " : " << paramMap["description"]->get<std::string>();
                                }
                                std::cout << "\n";
                            }
                        }
                    }
                } else {
                    std::cout << "\n";
                }
            }
            std::cout << "\n";
        }

        // Print classes
        Symbols::ObjectMap classesMap = infoMap["classes"];
        if (!classesMap.empty()) {
            std::cout << "=== Classes ===\n";
            for (const auto & [_, classInfo] : classesMap) {
                Symbols::ObjectMap classMap = classInfo;
                std::cout << "Class: " << classMap["name"]->get<std::string>() << "\n";

                // Print methods
                auto methodsMap = classMap["methods"]->get<Symbols::ObjectMap>();
                if (!methodsMap.empty()) {
                    std::cout << "  Methods:\n";
                    for (const auto & [_, methodInfo] : methodsMap) {
                        Symbols::ObjectMap methodMap = methodInfo;
                        std::cout << "    - " << methodMap["name"]->get<std::string>();
                        // Print method documentation if available
                        Symbols::ObjectMap docMap = methodMap["documentation"];
                        if (!docMap.empty()) {
                            if (docMap.contains("return_type")) {
                                std::cout << " -> " << docMap["return_type"]->get<std::string>();
                            }
                            if (docMap.contains("description") && !docMap["description"]->get<std::string>().empty()) {
                                std::cout << " : " << docMap["description"]->get<std::string>();
                            }
                            std::cout << "\n";

                            // Print parameters
                            if (docMap.contains("parameters")) {
                                Symbols::ObjectMap paramsMap = docMap["parameters"];
                                if (!paramsMap.empty()) {
                                    std::cout << "      Parameters:\n";
                                    for (const auto & [_, paramInfo] : paramsMap) {
                                        Symbols::ObjectMap paramMap = paramInfo;
                                        std::cout << "        - " << paramMap["name"]->get<std::string>();
                                        std::cout << " (" << paramMap["type"]->get<std::string>() << ")";
                                        if (paramMap["optional"]->get<bool>()) {
                                            std::cout << " [optional]";
                                        }
                                        if (paramMap["interpolate"]->get<bool>()) {
                                            std::cout << " [interpolate]";
                                        }
                                        if (paramMap.contains("description") && !paramMap["description"]->get<std::string>().empty()) {
                                            std::cout << " : " << paramMap["description"]->get<std::string>();
                                        }
                                        std::cout << "\n";
                                    }
                                }
                            }
                        } else {
                            std::cout << "\n";
                        }
                    }
                } else {
                    std::cout << "  No methods available.\n";
                }
                std::cout << "\n";
            }
        }

        return Symbols::ValuePtr::null();
    };
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
