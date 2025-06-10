#ifndef COMPILER_RUNTIME_LIBRARY_HPP
#define COMPILER_RUNTIME_LIBRARY_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Compiler {

/**
 * @brief Runtime function information
 */
struct RuntimeFunction {
    std::string name;
    std::string signature;
    std::string implementation;
    bool isBuiltin;
    
    // Default constructor
    RuntimeFunction() : name(""), signature(""), implementation(""), isBuiltin(false) {}
    
    RuntimeFunction(const std::string& n, const std::string& sig, const std::string& impl, bool builtin = true)
        : name(n), signature(sig), implementation(impl), isBuiltin(builtin) {}
};

/**
 * @brief Provides runtime support for compiled code
 * 
 * RuntimeLibrary is responsible for:
 * - Providing runtime functions for type conversions
 * - Memory management functions
 * - Built-in function implementations
 * - Integration with the VoidScript module system
 */
class RuntimeLibrary {
private:
    std::unordered_map<std::string, RuntimeFunction> functions_;
    std::vector<std::string> headers_;
    std::vector<std::string> implementations_;
    
public:
    /**
     * @brief Constructor
     */
    RuntimeLibrary();
    
    /**
     * @brief Destructor
     */
    ~RuntimeLibrary();
    
    /**
     * @brief Initialize the runtime library with built-in functions
     */
    void initialize();
    
    /**
     * @brief Add a runtime function
     * @param func Runtime function to add
     */
    void addFunction(const RuntimeFunction& func);
    
    /**
     * @brief Get runtime function by name
     * @param name Function name
     * @return Pointer to runtime function, nullptr if not found
     */
    const RuntimeFunction* getFunction(const std::string& name) const;
    
    /**
     * @brief Check if a function exists in the runtime library
     * @param name Function name
     * @return true if function exists
     */
    bool hasFunction(const std::string& name) const;
    
    /**
     * @brief Get all runtime functions
     * @return Map of all runtime functions
     */
    const std::unordered_map<std::string, RuntimeFunction>& getFunctions() const;
    
    /**
     * @brief Generate runtime library header code
     * @return Vector of header code lines
     */
    std::vector<std::string> generateHeaders() const;
    
    /**
     * @brief Generate runtime library implementation code
     * @return Vector of implementation code lines
     */
    std::vector<std::string> generateImplementations() const;
    
    /**
     * @brief Get function name for type conversion
     * @param fromType Source type
     * @param toType Target type
     * @return Function name for conversion, empty if not supported
     */
    std::string getTypeConversionFunction(Symbols::Variables::Type fromType,
                                        Symbols::Variables::Type toType) const;
    
    /**
     * @brief Get function name for type checking
     * @param type Type to check
     * @return Function name for type checking
     */
    std::string getTypeCheckFunction(Symbols::Variables::Type type) const;
    
    /**
     * @brief Get function name for memory allocation
     * @param type Type to allocate
     * @return Function name for memory allocation
     */
    std::string getAllocationFunction(Symbols::Variables::Type type) const;
    
    /**
     * @brief Get function name for memory deallocation
     * @param type Type to deallocate
     * @return Function name for memory deallocation
     */
    std::string getDeallocationFunction(Symbols::Variables::Type type) const;
    
    /**
     * @brief Get function name for built-in operations
     * @param operation Operation name (e.g., "print", "strlen", etc.)
     * @return Function name for the operation, empty if not supported
     */
    std::string getBuiltinFunction(const std::string& operation) const;

private:
    /**
     * @brief Add built-in type conversion functions
     */
    void addTypeConversionFunctions();
    
    /**
     * @brief Add built-in memory management functions
     */
    void addMemoryManagementFunctions();
    
    /**
     * @brief Add built-in utility functions
     */
    void addUtilityFunctions();
    
    /**
     * @brief Add built-in I/O functions
     */
    void addIOFunctions();
    
    /**
     * @brief Add built-in string functions
     */
    void addStringFunctions();
    
    /**
     * @brief Add built-in array functions
     */
    void addArrayFunctions();
    
    /**
     * @brief Add built-in object functions
     */
    void addObjectFunctions();
    
    /**
     * @brief Add runtime evaluation functions for variable and expression access
     */
    void addRuntimeEvaluationFunctions();
    
    /**
     * @brief Generate type name string
     * @param type VoidScript variable type
     * @return Type name as string
     */
    std::string typeToString(Symbols::Variables::Type type) const;
    
    /**
     * @brief Generate C type name
     * @param type VoidScript variable type
     * @return C type name
     */
    std::string typeToCType(Symbols::Variables::Type type) const;
};

} // namespace Compiler

#endif // COMPILER_RUNTIME_LIBRARY_HPP