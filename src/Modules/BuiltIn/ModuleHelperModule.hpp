// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "../BaseModule.hpp"
#include "../../Symbols/FunctionParameterInfo.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../utils.h"

namespace Modules {

/**
 * @brief Module providing helper functions for module information and documentation.
 */
class ModuleHelperModule : public BaseModule {
  public:
    /**
     * @brief Construct a new Module Helper Module object
     *
     * Initializes the ModuleHelper module for providing module information and documentation.
     */
    ModuleHelperModule() {
        setModuleName("ModuleHelper");
        setDescription("Provides reflection and introspection capabilities for modules, functions, classes, and methods in the VoidScript runtime");
        setBuiltIn(true);
    }

    /**
     * @brief Register all ModuleHelper functions with the VoidScript engine
     *
     * Registers module information and documentation functions for use in VoidScript code.
     */
    void registerFunctions() override;

  private:
    // ===== INTERNAL HELPER FUNCTIONS =====
    
    // Data gathering functions (lightweight, direct API calls)
    /**
     * @brief Gather all available module names
     *
     * Retrieves a list of all registered modules in the symbol container.
     *
     * @param sc Symbol container to query
     * @return std::vector<std::string> List of module names
     */
    static std::vector<std::string> gatherModuleNames(const Symbols::SymbolContainer * sc);
    
    /**
     * @brief Gather function names for a specific module
     *
     * Retrieves all function names registered under the specified module.
     *
     * @param moduleName Name of the module to query
     * @param sc Symbol container to query
     * @return std::vector<std::string> List of function names
     */
    static std::vector<std::string> gatherFunctionNames(const std::string & moduleName, const Symbols::SymbolContainer * sc);
    
    /**
     * @brief Gather class names for a specific module
     *
     * Retrieves all class names registered under the specified module.
     *
     * @param moduleName Name of the module to query
     * @param sc Symbol container to query
     * @return std::vector<std::string> List of class names
     */
    static std::vector<std::string> gatherClassNames(const std::string & moduleName, const Symbols::SymbolContainer * sc);
    
    /**
     * @brief Gather method names for a specific class
     *
     * Retrieves all method names for the specified class.
     *
     * @param className Name of the class to query
     * @param sc Symbol container to query
     * @return std::vector<std::string> List of method names
     */
    static std::vector<std::string> gatherMethodNames(const std::string & className, const Symbols::SymbolContainer * sc);
    
    // Documentation building functions (only called when detailed info is needed)
    /**
     * @brief Build parameter information map
     *
     * Creates a detailed object map containing parameter information
     * including name, type, description, and optionality.
     *
     * @param param Function parameter information to process
     * @return Symbols::ObjectMap Object containing parameter details
     */
    static Symbols::ObjectMap buildParameterInfoMap(const Symbols::FunctionParameterInfo & param);
    
    /**
     * @brief Build comprehensive function documentation
     *
     * Creates detailed documentation object for a function including
     * parameters, return type, and description.
     *
     * @param functionName Name of the function to document
     * @param sc Symbol container to query
     * @return Symbols::ObjectMap Object containing function documentation
     */
    static Symbols::ObjectMap buildFunctionDocumentation(const std::string & functionName, const Symbols::SymbolContainer * sc);
    
    /**
     * @brief Build comprehensive method documentation
     *
     * Creates detailed documentation object for a class method including
     * parameters, return type, and description.
     *
     * @param className Name of the class containing the method
     * @param methodName Name of the method to document
     * @param sc Symbol container to query
     * @return Symbols::ObjectMap Object containing method documentation
     */
    static Symbols::ObjectMap buildMethodDocumentation(const std::string & className, const std::string & methodName, const Symbols::SymbolContainer * sc);
    
    // Utility functions for object array creation
    /**
     * @brief Create VoidScript array from string vector
     *
     * Converts a vector of strings into a VoidScript-compatible array object.
     *
     * @param items Vector of strings to convert
     * @return Symbols::ValuePtr VoidScript array containing the strings
     */
    static Symbols::ValuePtr createStringArray(const std::vector<std::string> & items);
    
    /**
     * @brief Create VoidScript array from parameter info vector
     *
     * Converts a vector of function parameter information into a VoidScript-compatible array.
     *
     * @param parameters Vector of parameter information to convert
     * @return Symbols::ValuePtr VoidScript array containing parameter objects
     */
    static Symbols::ValuePtr createParameterArray(const std::vector<Symbols::FunctionParameterInfo> & parameters);

    // ===== PUBLIC API FUNCTIONS =====
    
    // 1. List Functions (return simple arrays)
    /**
     * @brief List all available modules
     *
     * Returns a simple array of all registered module names.
     *
     * @param args Function arguments (unused)
     * @return Symbols::ValuePtr Array of module names
     */
    static Symbols::ValuePtr ListModules(const FunctionArguments & args);
    
    /**
     * @brief List functions in a specific module
     *
     * Returns an array of function names for the specified module.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr Array of function names
     */
    static Symbols::ValuePtr ListModuleFunctions(const FunctionArguments & args);
    
    /**
     * @brief List classes in a specific module
     *
     * Returns an array of class names for the specified module.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr Array of class names
     */
    static Symbols::ValuePtr ListModuleClasses(const FunctionArguments & args);
    
    /**
     * @brief List methods in a specific class
     *
     * Returns an array of method names for the specified class.
     *
     * @param args Function arguments: (class_name)
     * @return Symbols::ValuePtr Array of method names
     */
    static Symbols::ValuePtr ListClassMethods(const FunctionArguments & args);
    
    // 2. Basic Info Functions (return flat objects with basic information)
    /**
     * @brief Get basic function information
     *
     * Returns basic information about a function including name, module, and parameter count.
     *
     * @param args Function arguments: (function_name)
     * @return Symbols::ValuePtr Object with basic function information
     */
    static Symbols::ValuePtr GetFunctionInfo(const FunctionArguments & args);
    
    /**
     * @brief Get basic class information
     *
     * Returns basic information about a class including name, module, and method count.
     *
     * @param args Function arguments: (class_name)
     * @return Symbols::ValuePtr Object with basic class information
     */
    static Symbols::ValuePtr GetClassInfo(const FunctionArguments & args);
    
    /**
     * @brief Get basic method information
     *
     * Returns basic information about a method including name, class, and parameter count.
     *
     * @param args Function arguments: (class_name, method_name)
     * @return Symbols::ValuePtr Object with basic method information
     */
    static Symbols::ValuePtr GetMethodInfo(const FunctionArguments & args);
    
    /**
     * @brief Get module summary
     *
     * Returns a summary of a module including function and class counts.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr Object with module summary information
     */
    static Symbols::ValuePtr GetModuleSummary(const FunctionArguments & args);

    /**
     * @brief Get module description
     *
     * Returns the description of the specified module.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr String containing module description
     */
    static Symbols::ValuePtr GetModuleDescription(const FunctionArguments & args);
    
    // 3. Detailed Info Functions (return comprehensive objects with full documentation)
    /**
     * @brief Get detailed function information
     *
     * Returns comprehensive documentation for a function including full parameter
     * information, return type, and description.
     *
     * @param args Function arguments: (function_name)
     * @return Symbols::ValuePtr Object with detailed function documentation
     */
    static Symbols::ValuePtr GetFunctionDetails(const FunctionArguments & args);
    
    /**
     * @brief Get detailed class information
     *
     * Returns comprehensive documentation for a class including all methods
     * and their detailed information.
     *
     * @param args Function arguments: (class_name)
     * @return Symbols::ValuePtr Object with detailed class documentation
     */
    static Symbols::ValuePtr GetClassDetails(const FunctionArguments & args);
    
    /**
     * @brief Get detailed method information
     *
     * Returns comprehensive documentation for a method including full parameter
     * information, return type, and description.
     *
     * @param args Function arguments: (class_name, method_name)
     * @return Symbols::ValuePtr Object with detailed method documentation
     */
    static Symbols::ValuePtr GetMethodDetails(const FunctionArguments & args);
    
    /**
     * @brief Get detailed module information
     *
     * Returns comprehensive documentation for a module including all functions,
     * classes, and their detailed information.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr Object with detailed module documentation
     */
    static Symbols::ValuePtr GetModuleDetails(const FunctionArguments & args);
    
    // 4. Utility Functions (return boolean existence checks)
    /**
     * @brief Check if module exists
     *
     * Returns true if the specified module is registered.
     *
     * @param args Function arguments: (module_name)
     * @return Symbols::ValuePtr Boolean indicating module existence
     */
    static Symbols::ValuePtr ModuleExists(const FunctionArguments & args);
    
    /**
     * @brief Check if function exists
     *
     * Returns true if the specified function is registered.
     *
     * @param args Function arguments: (function_name)
     * @return Symbols::ValuePtr Boolean indicating function existence
     */
    static Symbols::ValuePtr FunctionExists(const FunctionArguments & args);
    
    /**
     * @brief Check if class exists
     *
     * Returns true if the specified class is registered.
     *
     * @param args Function arguments: (class_name)
     * @return Symbols::ValuePtr Boolean indicating class existence
     */
    static Symbols::ValuePtr ClassExists(const FunctionArguments & args);
    
    /**
     * @brief Check if method exists
     *
     * Returns true if the specified method exists in the given class.
     *
     * @param args Function arguments: (class_name, method_name)
     * @return Symbols::ValuePtr Boolean indicating method existence
     */
    static Symbols::ValuePtr MethodExists(const FunctionArguments & args);
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
