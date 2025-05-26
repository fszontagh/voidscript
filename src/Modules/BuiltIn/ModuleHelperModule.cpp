// ModuleHelperModule.cpp
#include "ModuleHelperModule.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/FunctionParameterInfo.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "utils.h"

namespace Modules {

void ModuleHelperModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> params = {};

    // List all modules
    REGISTER_FUNCTION("module_list", Symbols::Variables::Type::OBJECT, params,
                      "List all available modules with their registered entities",
                      ModuleHelperModule::ModuleList);

    // Check if module exists
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to check", false, false }
    };
    REGISTER_FUNCTION("module_exists", Symbols::Variables::Type::BOOLEAN, params,
                      "Check if a module with the given name exists",
                      ModuleHelperModule::ModuleExists);

    // Get module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to get info for", false, false }
    };
    REGISTER_FUNCTION("module_info", Symbols::Variables::Type::OBJECT, params,
                      "Get detailed information about a specific module or all modules if 'all' is passed",
                      ModuleHelperModule::ModuleInfo);

    // Print detailed module info
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the module to print info for", false, false }
    };

    REGISTER_FUNCTION("module_print_info", Symbols::Variables::Type::NULL_TYPE, params,
                      "Print detailed information about a module in a formatted way",
                      ModuleHelperModule::ModulePrintInfo);

    // Get function documentation
    params = {
        { "name", Symbols::Variables::Type::STRING, "Name of the function to get documentation for", false, false }
    };
    REGISTER_FUNCTION("function_doc", Symbols::Variables::Type::OBJECT, params,
                      "Get documentation for a specific function including parameters and return type",
                      ModuleHelperModule::FunctionInfo);
}

Symbols::ValuePtr ModuleHelperModule::ModuleList(const FunctionArguments & args) {
    if (!args.empty()) {
        throw std::runtime_error("module_list expects no arguments");
    }

    auto * sc = Symbols::SymbolContainer::instance();

    // Get all registered module names
    auto moduleNames = sc->getModuleNames();

    // Create object map to hold module info objects using module names as keys
    Symbols::ObjectMap moduleArray;

    for (const auto & moduleName : moduleNames) {
        auto        module      = sc->getModule(moduleName);
        std::string path        = "/modules/" + moduleName;
        moduleArray[moduleName] = ModuleHelperModule::buildModuleInfoMap(module, path, sc);
    }

    return Symbols::ValuePtr(moduleArray);
}

Symbols::ValuePtr ModuleHelperModule::ModuleExists(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("module_exists expects exactly one string argument");
    }

    std::string moduleName = args[0];
    auto *      sc         = Symbols::SymbolContainer::instance();

    return sc->hasModule(moduleName);
}

Symbols::ValuePtr ModuleHelperModule::ModuleInfo(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("module_info expects exactly one string argument");
    }

    std::string moduleName = args[0];
    auto *      sc         = Symbols::SymbolContainer::instance();

    // Special case: if moduleName is "all", return info for all modules
    if (moduleName == "all") {
        return ModuleHelperModule::buildModuleInfoMap(nullptr, "/modules", sc);
    }

    // Check if the specific module exists
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }

    auto        module = sc->getModule(moduleName);
    std::string path   = "/modules/" + moduleName;
    return ModuleHelperModule::buildModuleInfoMap(module, path, sc);
}

Symbols::ValuePtr ModuleHelperModule::FunctionInfo(const FunctionArguments & args) {
    if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("function_doc expects exactly one string argument");
    }

    const std::string functionName = args[0];
    auto *            sc           = Symbols::SymbolContainer::instance();
    return buildFunctionDocMap(functionName, sc);
}

Symbols::ValuePtr ModuleHelperModule::ModulePrintInfo(const FunctionArguments & args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("module_print_info expects exactly one string argument");
    }

    std::string moduleName = args[0];
    auto *      sc         = Symbols::SymbolContainer::instance();

    // Check if the module exists
    if (!sc->hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }

    auto        module = sc->getModule(moduleName);
    std::string path   = "/modules/" + moduleName;

    // Get module info
    auto infoMap = ModuleHelperModule::buildModuleInfoMap(module, path, sc);

    // Print module header
    std::cout << "=== Module Information ===\n";
    std::cout << "Module name: " << moduleName << "\n";
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
                            if (paramMap.contains("description") &&
                                !paramMap["description"]->get<std::string>().empty()) {
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
                                    if (paramMap.contains("description") &&
                                        !paramMap["description"]->get<std::string>().empty()) {
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
}

}  // namespace Modules
