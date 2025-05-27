// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/FunctionParameterInfo.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
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
            // Filter by module if one is specified
            if (module == nullptr || sc->getClassModule(cls) == module) {
                classesMap[cls] = buildClassInfoMap(cls, sc);
            }
        }

        int fi = 0;
        for (const auto & fn : funcNames) {
            // For functions, we'll include all since we don't track function-to-module mapping in SymbolContainer
            // This could be enhanced by adding function-to-module tracking if needed
            if (module == nullptr) {
                // Include all functions when no specific module is requested
                funcsMap[fn] = buildFunctionInfoMap(fn, sc);
            }
            // When a specific module is requested, we can't easily filter functions
            // since SymbolContainer doesn't track which module registered each function
            // This could be improved by enhancing the function registration to track module ownership
        }

        Symbols::ObjectMap varsMap;

        Symbols::ObjectMap infoMap;
        infoMap["name"]          = module ? module->name() : utils::get_filename_stem(path);
        infoMap["path"]          = path;
        infoMap["classes"]       = classesMap;
        infoMap["functions"]     = funcsMap;
        infoMap["variables"]     = varsMap;
        infoMap["documentation"] = Symbols::ObjectMap();

        return infoMap;
    }

    // Helper methods for building entity information
    static Symbols::ObjectMap buildClassInfoMap(const std::string & className, const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap classInfo;
        classInfo["name"] = className;

        Symbols::ObjectMap methodsMap;
        auto               methodNames = sc->getMethodNames(className);
        int                mi          = 0;
        for (const auto & methodName : methodNames) {
            methodsMap[methodName] = buildMethodInfoMap(className, methodName, sc);
        }
        classInfo["methods"] = methodsMap;

        return classInfo;
    };

    static Symbols::ObjectMap buildFunctionInfoMap(const std::string &              functionName,
                                                   const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap funcInfo;
        funcInfo["name"]          = functionName;
        funcInfo["documentation"] = buildFunctionDocMap(functionName, sc);

        return funcInfo;
    }

    static Symbols::ObjectMap buildMethodInfoMap(const std::string & className, const std::string & methodName,
                                                 const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap methodInfo;
        methodInfo["name"]  = methodName;
        methodInfo["class"] = className;

        // Try to get method information from SymbolContainer
        Symbols::ObjectMap docInfo;
        std::string        qualifiedMethodName = className + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName;

        // First, try to get method documentation if it exists
        auto docInfoFromFunc = ModuleHelperModule::buildFunctionDocMap(qualifiedMethodName, sc);
        if (!docInfoFromFunc.empty()) {
            docInfo = docInfoFromFunc;
        } else {
            // Build documentation from available method information
            docInfo["name"] = qualifiedMethodName;

            // Try to get return type from SymbolContainer
            try {
                auto returnType = sc->getMethodReturnType(className, methodName);
                if (returnType != Symbols::Variables::Type::NULL_TYPE) {
                    docInfo["return_type"] = Symbols::Variables::TypeToString(returnType);
                }

                // Try to get parameters from SymbolContainer
                auto parameters = sc->getMethodParameters(className, methodName);
                if (!parameters.empty()) {
                    Symbols::ObjectMap paramsMap;
                    for (const auto & param : parameters) {
                        paramsMap[param.name] = buildParameterInfoMap(param);
                    }
                    docInfo["parameters"] = paramsMap;
                }
            } catch (...) {
                // Method not found, continue with minimal info
            }

            // Set empty description if not available
            if (!docInfo.contains("description")) {
                docInfo["description"] = "";
            }
        }

        methodInfo["documentation"] = docInfo;
        return methodInfo;
    }

    // Helper methods for building documentation
    static Symbols::ObjectMap buildParameterInfoMap(const Symbols::FunctionParameterInfo & param) {
        Symbols::ObjectMap paramInfo;
        paramInfo["name"]        = param.name;
        paramInfo["type"]        = Symbols::Variables::TypeToString(param.type);
        paramInfo["description"] = param.description;
        paramInfo["optional"]    = param.optional;
        paramInfo["interpolate"] = param.interpolate;
        return paramInfo;
    }

    static Symbols::ObjectMap buildFunctionDocMap(const std::string &              functionName,
                                                  const Symbols::SymbolContainer * sc) {
        Symbols::ObjectMap docInfo;

        // First try to find it in the functions
        if (sc->hasFunction(functionName)) {
            auto funcDoc           = sc->getFunctionDoc(functionName);
            docInfo["name"]        = funcDoc.name;
            docInfo["description"] = funcDoc.description;
            docInfo["return_type"] = Symbols::Variables::TypeToString(funcDoc.returnType);
            Symbols::ObjectMap paramsMap;
            for (const auto & param : funcDoc.parameterList) {
                paramsMap[param.name] = buildParameterInfoMap(param);
            }
            docInfo["parameters"] = paramsMap;
            return docInfo;
        }

        // If not found in functions, try to find it in methods (for qualified method names)
        // Parse qualified method name (className::methodName)
        size_t scopePos = functionName.find("::");
        if (scopePos != std::string::npos) {
            std::string className  = functionName.substr(0, scopePos);
            std::string methodName = functionName.substr(scopePos + 2);

            try {
                if (sc->hasClass(className) && sc->hasMethod(className, methodName)) {
                    auto returnType = sc->getMethodReturnType(className, methodName);
                    auto parameters = sc->getMethodParameters(className, methodName);

                    docInfo["name"]        = functionName;
                    docInfo["description"] = "";  // No description available
                    docInfo["return_type"] = Symbols::Variables::TypeToString(returnType);

                    Symbols::ObjectMap paramsMap;
                    for (const auto & param : parameters) {
                        paramsMap[param.name] = buildParameterInfoMap(param);
                    }
                    docInfo["parameters"] = paramsMap;
                    return docInfo;
                }
            } catch (...) {
                // Method not found
            }
        }

        return docInfo;
    }

    static Symbols::ValuePtr FunctionInfo(const FunctionArguments & args);

    static Symbols::ValuePtr ModuleList(const FunctionArguments & args);

    static Symbols::ValuePtr ModuleExists(const FunctionArguments & args);

    static Symbols::ValuePtr ModuleInfo(const FunctionArguments & args);

    static Symbols::ValuePtr ModulePrintInfo(const FunctionArguments & args);
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
