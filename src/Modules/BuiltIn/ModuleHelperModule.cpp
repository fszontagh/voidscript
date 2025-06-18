// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "../../Symbols/FunctionParameterInfo.hpp"
#include "../../Symbols/RegistrationMacros.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../utils.h"

namespace Modules {

void ModuleHelperModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> params;

    // ===== 1. LIST FUNCTIONS =====
    
    // List all modules
    params = {};
    REGISTER_FUNCTION("list_modules", Symbols::Variables::Type::OBJECT, params,
                      "Get list of all loaded module names",
                      ModuleHelperModule::ListModules);

    // List module functions
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module", false, false }
    };
    REGISTER_FUNCTION("list_module_functions", Symbols::Variables::Type::OBJECT, params,
                      "Get list of function names for a specific module",
                      ModuleHelperModule::ListModuleFunctions);

    // List module classes
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module", false, false }
    };
    REGISTER_FUNCTION("list_module_classes", Symbols::Variables::Type::OBJECT, params,
                      "Get list of class names for a specific module",
                      ModuleHelperModule::ListModuleClasses);

    // List class methods
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false }
    };
    REGISTER_FUNCTION("list_class_methods", Symbols::Variables::Type::OBJECT, params,
                      "Get list of method names for a specific class",
                      ModuleHelperModule::ListClassMethods);

    // ===== 2. BASIC INFO FUNCTIONS =====
    
    // Get function basic info
    params = {
        { "function_name", Symbols::Variables::Type::STRING, "Name of the function", false, false }
    };
    REGISTER_FUNCTION("get_function_info", Symbols::Variables::Type::OBJECT, params,
                      "Get basic information about a function",
                      ModuleHelperModule::GetFunctionInfo);

    // Get class basic info
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false }
    };
    REGISTER_FUNCTION("get_class_details", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed information about a class",
                      ModuleHelperModule::GetClassDetails);

    // Get method basic info
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false },
        { "method_name", Symbols::Variables::Type::STRING, "Name of the method", false, false }
    };
    REGISTER_FUNCTION("get_method_info", Symbols::Variables::Type::OBJECT, params,
                      "Get basic information about a method",
                      ModuleHelperModule::GetMethodInfo);

    // Get module summary
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module", false, false }
    };
    REGISTER_FUNCTION("get_module_summary", Symbols::Variables::Type::OBJECT, params,
                      "Get summary information about a module",
                      ModuleHelperModule::GetModuleSummary);

    // Get module description
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module", false, false }
    };
    REGISTER_FUNCTION("get_module_description", Symbols::Variables::Type::STRING, params,
                      "Get the description of a module",
                      ModuleHelperModule::GetModuleDescription);

    // ===== 3. DETAILED INFO FUNCTIONS =====
    
    // Get function detailed info
    params = {
        { "function_name", Symbols::Variables::Type::STRING, "Name of the function", false, false }
    };
    REGISTER_FUNCTION("get_function_details", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed documentation for a function",
                      ModuleHelperModule::GetFunctionDetails);

    // Get class detailed info
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false }
    };
    REGISTER_FUNCTION("get_class_details", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed information about a class",
                      ModuleHelperModule::GetClassDetails);

    // Get method detailed info
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false },
        { "method_name", Symbols::Variables::Type::STRING, "Name of the method", false, false }
    };
    REGISTER_FUNCTION("get_method_details", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed documentation for a method",
                      ModuleHelperModule::GetMethodDetails);

    // Get module detailed info
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module", false, false }
    };
    REGISTER_FUNCTION("get_module_details", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed information about a module",
                      ModuleHelperModule::GetModuleDetails);

    // ===== 4. UTILITY FUNCTIONS =====
    
    // Check if module exists
    params = {
        { "module_name", Symbols::Variables::Type::STRING, "Name of the module to check", false, false }
    };
    REGISTER_FUNCTION("module_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a module with the given name exists",
                      ModuleHelperModule::ModuleExists);

    // Check if function exists
    params = {
        { "function_name", Symbols::Variables::Type::STRING, "Name of the function to check", false, false }
    };
    REGISTER_FUNCTION("function_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a function with the given name exists",
                      ModuleHelperModule::FunctionExists);

    // Check if class exists
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class to check", false, false }
    };
    REGISTER_FUNCTION("class_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a class with the given name exists",
                      ModuleHelperModule::ClassExists);

    // Check if method exists
    params = {
        { "class_name", Symbols::Variables::Type::STRING, "Name of the class", false, false },
        { "method_name", Symbols::Variables::Type::STRING, "Name of the method to check", false, false }
    };
    REGISTER_FUNCTION("method_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a method with the given name exists in the specified class",
                      ModuleHelperModule::MethodExists);
}

// ===== INTERNAL HELPER FUNCTIONS =====

std::vector<std::string> ModuleHelperModule::gatherModuleNames(const Symbols::SymbolContainer * sc) {
    return sc->getModuleNames();
}

std::vector<std::string> ModuleHelperModule::gatherFunctionNames(const std::string & moduleName, const Symbols::SymbolContainer * sc) {
    auto module = sc->getModule(moduleName);
    if (!module) {
        return {};
    }
    return sc->getFunctionNamesByModule(module);
}

std::vector<std::string> ModuleHelperModule::gatherClassNames(const std::string & moduleName, const Symbols::SymbolContainer * sc) {
    auto module = sc->getModule(moduleName);
    if (!module) {
        return {};
    }
    
    std::vector<std::string> result;
    auto allClassNames = sc->getClassNames();
    
    for (const auto & className : allClassNames) {
        if (sc->getClassModule(className) == module) {
            result.push_back(className);
        }
    }
    
    return result;
}

std::vector<std::string> ModuleHelperModule::gatherMethodNames(const std::string & className, const Symbols::SymbolContainer * sc) {
    return sc->getMethodNames(className);
}

Symbols::ObjectMap ModuleHelperModule::buildParameterInfoMap(const Symbols::FunctionParameterInfo & param) {
    Symbols::ObjectMap paramInfo;
    paramInfo["name"] = param.name;
    paramInfo["type"] = Symbols::Variables::TypeToString(param.type);
    paramInfo["description"] = param.description;
    paramInfo["optional"] = param.optional;
    paramInfo["interpolate"] = param.interpolate;
    return paramInfo;
}

Symbols::ObjectMap ModuleHelperModule::buildFunctionDocumentation(const std::string & functionName, const Symbols::SymbolContainer * sc) {
    Symbols::ObjectMap docInfo;
    
    if (sc->hasFunction(functionName)) {
        auto funcDoc = sc->getFunctionDoc(functionName);
        docInfo["name"] = funcDoc.name;
        docInfo["description"] = funcDoc.description;
        docInfo["return_type"] = Symbols::Variables::TypeToString(funcDoc.returnType);
        docInfo["parameters"] = createParameterArray(funcDoc.parameterList);
        
        // Add module information
        auto functionModule = sc->getFunctionModule(functionName);
        docInfo["module"] = functionModule ? functionModule->name() : "";
    }
    
    return docInfo;
}

Symbols::ObjectMap ModuleHelperModule::buildMethodDocumentation(const std::string & className, const std::string & methodName, const Symbols::SymbolContainer * sc) {
    Symbols::ObjectMap docInfo;
    std::string qualifiedMethodName = className + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName;
    
    // Try to get method documentation from the class registry (where native methods are stored)
    if (sc->hasClass(className) && sc->hasMethod(className, methodName)) {
        try {
            auto returnType = sc->getMethodReturnType(className, methodName);
            auto parameters = sc->getMethodParameters(className, methodName);
            
            docInfo["name"] = methodName;
            docInfo["class"] = className;
            docInfo["qualified_name"] = qualifiedMethodName;
            docInfo["return_type"] = Symbols::Variables::TypeToString(returnType);
            docInfo["is_private"] = sc->isMethodPrivate(className, methodName);
            docInfo["parameters"] = createParameterArray(parameters);
            
            // Retrieve method documentation from the class registry's method info
            const auto & classInfo = sc->getClassInfo(className);
            for (const auto & method : classInfo.methods) {
                if (method.name == methodName) {
                    docInfo["description"] = method.documentation.description;
                    break;
                }
            }
            
        } catch (...) {
            // Method not found or error accessing method info
        }
    }
    
    return docInfo;
}

Symbols::ValuePtr ModuleHelperModule::createStringArray(const std::vector<std::string> & items) {
    Symbols::ObjectMap arrayMap;
    
    for (size_t i = 0; i < items.size(); ++i) {
        arrayMap[std::to_string(i)] = items[i];
    }
    
    return Symbols::ValuePtr(arrayMap);
}

Symbols::ValuePtr ModuleHelperModule::createParameterArray(const std::vector<Symbols::FunctionParameterInfo> & parameters) {
    Symbols::ObjectMap arrayMap;
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        arrayMap[std::to_string(i)] = buildParameterInfoMap(parameters[i]);
    }
    
    return Symbols::ValuePtr(arrayMap);
}

// ===== 1. LIST FUNCTIONS =====

Symbols::ValuePtr ModuleHelperModule::ListModules(const FunctionArguments & args) {
    if (!args.empty()) {
        throw std::runtime_error("list_modules expects no arguments");
    }
    
    auto * sc = Symbols::SymbolContainer::instance();
    auto moduleNames = gatherModuleNames(sc);
    
    return createStringArray(moduleNames);
}

Symbols::ValuePtr ModuleHelperModule::ListModuleFunctions(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("list_module_functions expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
    
    auto functionNames = gatherFunctionNames(moduleName, sc);
    return createStringArray(functionNames);
}

Symbols::ValuePtr ModuleHelperModule::ListModuleClasses(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("list_module_classes expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
    
    auto classNames = gatherClassNames(moduleName, sc);
    return createStringArray(classNames);
}

Symbols::ValuePtr ModuleHelperModule::ListClassMethods(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("list_class_methods expects exactly one string argument");
    }
    
    std::string className = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasClass(className)) {
        throw std::runtime_error("Class not found: " + className);
    }
    
    auto methodNames = gatherMethodNames(className, sc);
    return createStringArray(methodNames);
}

// ===== 2. BASIC INFO FUNCTIONS =====

Symbols::ValuePtr ModuleHelperModule::GetFunctionInfo(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_function_info expects exactly one string argument");
    }
    
    std::string functionName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasFunction(functionName)) {
        throw std::runtime_error("Function not found: " + functionName);
    }
    
    Symbols::ObjectMap info;
    info["name"] = functionName;
    
    auto returnType = sc->getFunctionReturnType(functionName);
    info["return_type"] = Symbols::Variables::TypeToString(returnType);
    
    auto funcDoc = sc->getFunctionDoc(functionName);
    info["parameter_count"] = static_cast<int>(funcDoc.parameterList.size());
    
    auto functionModule = sc->getFunctionModule(functionName);
    info["module"] = functionModule ? functionModule->name() : "";
    
    return Symbols::ValuePtr(info);
}

Symbols::ValuePtr ModuleHelperModule::GetClassInfo(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_class_info expects exactly one string argument");
    }
    
    std::string className = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasClass(className)) {
        throw std::runtime_error("Class not found: " + className);
    }
    
    const auto & classInfo = sc->getClassInfo(className);
    
    Symbols::ObjectMap info;
    info["name"] = className;
    info["method_count"] = static_cast<int>(classInfo.methods.size());
    info["property_count"] = static_cast<int>(classInfo.properties.size());
    info["parent_class"] = classInfo.parentClass;
    info["module"] = classInfo.module ? classInfo.module->name() : "";
    
    return Symbols::ValuePtr(info);
}

Symbols::ValuePtr ModuleHelperModule::GetMethodInfo(const FunctionArguments & args) {
    if (args.size() != 2 || args[0]->getType() != Symbols::Variables::Type::STRING ||
        args[1]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_method_info expects exactly two string arguments");
    }
    
    std::string className = args[0]->toString();
    std::string methodName = args[1]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasClass(className)) {
        throw std::runtime_error("Class not found: " + className);
    }
    
    if (!sc->hasMethod(className, methodName)) {
        throw std::runtime_error("Method not found: " + className + "::" + methodName);
    }
    
    Symbols::ObjectMap info;
    info["name"] = methodName;
    info["class"] = className;
    
    auto returnType = sc->getMethodReturnType(className, methodName);
    info["return_type"] = Symbols::Variables::TypeToString(returnType);
    
    auto parameters = sc->getMethodParameters(className, methodName);
    info["parameter_count"] = static_cast<int>(parameters.size());
    
    info["is_private"] = sc->isMethodPrivate(className, methodName);
    
    return Symbols::ValuePtr(info);
}

Symbols::ValuePtr ModuleHelperModule::GetModuleSummary(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_module_summary expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
    
    auto functionNames = gatherFunctionNames(moduleName, sc);
    auto classNames = gatherClassNames(moduleName, sc);
    
    Symbols::ObjectMap info;
    info["name"] = moduleName;
    info["description"] = sc->getModuleDescription(moduleName);
    info["function_count"] = static_cast<int>(functionNames.size());
    info["class_count"] = static_cast<int>(classNames.size());
    info["is_built_in"] = true;  // All modules in our system are considered built-in
    
    return Symbols::ValuePtr(info);
}

// ===== 3. DETAILED INFO FUNCTIONS =====

Symbols::ValuePtr ModuleHelperModule::GetFunctionDetails(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_function_details expects exactly one string argument");
    }
    
    std::string functionName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasFunction(functionName)) {
        throw std::runtime_error("Function not found: " + functionName);
    }
    
    auto documentation = buildFunctionDocumentation(functionName, sc);
    return Symbols::ValuePtr(documentation);
}

Symbols::ValuePtr ModuleHelperModule::GetClassDetails(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_class_details expects exactly one string argument");
    }
    
    std::string className = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasClass(className)) {
        throw std::runtime_error("Class not found: " + className);
    }
    
    const auto & classInfo = sc->getClassInfo(className);
    auto methodNames = gatherMethodNames(className, sc);
    
    Symbols::ObjectMap details;
    details["name"] = className;
    details["parent_class"] = classInfo.parentClass;
    details["module"] = classInfo.module ? classInfo.module->name() : "";
    details["methods"] = createStringArray(methodNames);
    
    // Create property names array
    std::vector<std::string> propertyNames;
    for (const auto & prop : classInfo.properties) {
        propertyNames.push_back(prop.name);
    }
    details["properties"] = createStringArray(propertyNames);
    
    // Create static property names array
    std::vector<std::string> staticPropertyNames;
    for (const auto & [propName, _] : classInfo.staticProperties) {
        staticPropertyNames.push_back(propName);
    }
    details["static_properties"] = createStringArray(staticPropertyNames);
    
    return Symbols::ValuePtr(details);
}

Symbols::ValuePtr ModuleHelperModule::GetMethodDetails(const FunctionArguments & args) {
    if (args.size() != 2 || args[0]->getType() != Symbols::Variables::Type::STRING ||
        args[1]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_method_details expects exactly two string arguments");
    }
    
    std::string className = args[0]->toString();
    std::string methodName = args[1]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasClass(className)) {
        throw std::runtime_error("Class not found: " + className);
    }
    
    if (!sc->hasMethod(className, methodName)) {
        throw std::runtime_error("Method not found: " + className + "::" + methodName);
    }
    
    auto documentation = buildMethodDocumentation(className, methodName, sc);
    return Symbols::ValuePtr(documentation);
}

Symbols::ValuePtr ModuleHelperModule::GetModuleDetails(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_module_details expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
    
    auto functionNames = gatherFunctionNames(moduleName, sc);
    auto classNames = gatherClassNames(moduleName, sc);
    
    // Count total methods across all classes
    int totalMethods = 0;
    for (const auto & className : classNames) {
        auto methodNames = gatherMethodNames(className, sc);
        totalMethods += static_cast<int>(methodNames.size());
    }
    
    Symbols::ObjectMap details;
    details["name"] = moduleName;
    details["description"] = sc->getModuleDescription(moduleName);
    details["is_built_in"] = true;  // All modules in our system are considered built-in
    details["functions"] = createStringArray(functionNames);
    details["classes"] = createStringArray(classNames);
    details["total_methods"] = totalMethods;
    
    return Symbols::ValuePtr(details);
}

// ===== 4. UTILITY FUNCTIONS =====

Symbols::ValuePtr ModuleHelperModule::ModuleExists(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("module_exists expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    
    // Thread-local guard to prevent recursive calls during SymbolContainer access
    static thread_local bool inModuleCheck = false;
    if (inModuleCheck) {
        // If we're already checking, fall back to hardcoded list
        static const std::vector<std::string> knownModules = {
            "Math", "String", "File", "Json", "Array", "Print", "VariableHelpers",
            "Conversion", "ModuleHelper", "Readline", "Curl", "Format"
        };
        
        for (const auto& module : knownModules) {
            if (module == moduleName) {
                return Symbols::ValuePtr(true);
            }
        }
        return Symbols::ValuePtr(false);
    }
    
    // Set guard and try to access SymbolContainer safely
    inModuleCheck = true;
    try {
        auto * sc = Symbols::SymbolContainer::instance();
        bool result = sc->hasModule(moduleName);
        inModuleCheck = false;
        return Symbols::ValuePtr(result);
    } catch (...) {
        // If SymbolContainer access fails, fall back to hardcoded list
        inModuleCheck = false;
        static const std::vector<std::string> knownModules = {
            "Math", "String", "File", "Json", "Array", "Print", "VariableHelpers",
            "Conversion", "ModuleHelper", "Readline", "Curl", "Format"
        };
        
        for (const auto& module : knownModules) {
            if (module == moduleName) {
                return Symbols::ValuePtr(true);
            }
        }
        return Symbols::ValuePtr(false);
    }
}

Symbols::ValuePtr ModuleHelperModule::FunctionExists(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("function_exists expects exactly one string argument");
    }
    
    std::string functionName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    return Symbols::ValuePtr(sc->hasFunction(functionName));
}

Symbols::ValuePtr ModuleHelperModule::ClassExists(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("class_exists expects exactly one string argument");
    }
    
    std::string className = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    return Symbols::ValuePtr(sc->hasClass(className));
}

Symbols::ValuePtr ModuleHelperModule::MethodExists(const FunctionArguments & args) {
    if (args.size() != 2 || args[0]->getType() != Symbols::Variables::Type::STRING ||
        args[1]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("method_exists expects exactly two string arguments");
    }
    
    std::string className = args[0]->toString();
    std::string methodName = args[1]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    return Symbols::ValuePtr(sc->hasClass(className) && sc->hasMethod(className, methodName));
}

Symbols::ValuePtr ModuleHelperModule::GetModuleDescription(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("get_module_description expects exactly one string argument");
    }
    
    std::string moduleName = args[0]->toString();
    auto * sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
    
    std::string description = sc->getModuleDescription(moduleName);
    return Symbols::ValuePtr(description);
}

}  // namespace Modules
