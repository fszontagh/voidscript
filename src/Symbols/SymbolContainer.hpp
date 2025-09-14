#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include <algorithm> // For std::find
#include <atomic>  // Required for std::atomic
#include <functional>
#include <iostream> // For std::cerr, std::endl
#include <memory>
#include <sstream>  // For std::stringstream
#include <stdexcept>
#include <thread>   // For thread_local
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Symbols/BaseSymbol.hpp"
#include "Symbols/FunctionParameterInfo.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableSymbol.hpp"
#include "Symbols/VariableTypes.hpp"
#include "SymbolTable.hpp"
#include "../Modules/BaseModule.hpp"

// Forward declarations to avoid circular dependencies
namespace Parser {
class ParsedExpression;
using ParsedExpressionPtr = std::shared_ptr<ParsedExpression>;
}  // namespace Parser

namespace Modules {
class BaseModule;

// Custom deleter for BaseModule to work with forward declaration
struct BaseModuleDeleter {
    void operator()(BaseModule* module) const;
};

// Type alias for easier usage
using BaseModulePtr = std::unique_ptr<BaseModule, BaseModuleDeleter>;

// Helper function to create BaseModulePtr from standard unique_ptr
BaseModulePtr make_base_module_ptr(std::unique_ptr<BaseModule> module);
}

namespace Symbols {

/**
 * @brief Documentation structure for functions and methods.
 */
struct FunctionDoc {
    std::string                        name;                                     // function/method name
    Variables::Type                    returnType = Variables::Type::NULL_TYPE;  // return type
    std::vector<FunctionParameterInfo> parameterList;                            // parameter list
    std::string                        description;                              // short description
};

/**
 * @brief Information about a class property
 */
struct PropertyInfo {
    std::string                 name;
    Variables::Type             type;
    Parser::ParsedExpressionPtr defaultValueExpr;
    bool                        isPrivate = false;
};

/**
 * @brief Information about a class method
 */
struct MethodInfo {
    std::string                                            name;
    std::string                                            qualifiedName;
    Variables::Type                                        returnType;
    std::vector<FunctionParameterInfo>                     parameters;
    bool                                                   isPrivate = false;
    std::function<ValuePtr(const std::vector<ValuePtr> &)> nativeImplementation;
    FunctionDoc                                            documentation;
};

/**
 * @brief Information about a class
 */
struct ClassInfo {
    std::string                               name;
    std::string                               parentClass;  // For inheritance
    std::vector<PropertyInfo>                 properties;
    std::vector<MethodInfo>                   methods;
    std::unordered_map<std::string, ValuePtr> staticProperties;
    Modules::BaseModule *                     module = nullptr;  // Module that defined this class
};

using FunctionArguments = const std::vector<ValuePtr>;
using CallbackFunction  = std::function<ValuePtr(FunctionArguments &)>;

class SymbolContainer {
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> scopes_;
    // Stack of active scope names (supports nested scope entry)
    std::vector<std::string>                                      scopeStack_;

    // For unique call frame IDs
    inline static std::atomic<unsigned long long> next_call_frame_id_ = 0;

    // For singleton initialization
    static std::string initial_scope_name_for_singleton_;
    static bool        is_initialized_for_singleton_;

    // Class registry
    std::unordered_map<std::string, ClassInfo> classes_;

    // Function registry
    std::unordered_map<std::string, CallbackFunction> functions_;
    std::unordered_map<std::string, FunctionDoc>      functionDocs_;
    
    // Function-to-module mapping
    std::unordered_map<std::string, Modules::BaseModule*> functionModules_;

    // Module storage by name
    std::unordered_map<std::string, Modules::BaseModulePtr> modules_;

    // Module descriptions storage
    std::unordered_map<std::string, std::string> moduleDescriptions_;

    // Current module being registered (for macro support)
    Modules::BaseModule * currentModule_ = nullptr;

    // Private constructor
    explicit SymbolContainer(const std::string & default_scope_name);

  public:
    SymbolContainer(const SymbolContainer &)             = delete;
    SymbolContainer & operator=(const SymbolContainer &) = delete;

    static void initialize(const std::string & initial_scope_name) {
        if (initial_scope_name.empty()) {
            throw std::invalid_argument("Initial scope name for SymbolContainer cannot be empty.");
        }
        if (is_initialized_for_singleton_) {
            // Allow re-initialization for now. Could be an error in stricter scenarios.
        }
        initial_scope_name_for_singleton_ = initial_scope_name;
        is_initialized_for_singleton_     = true;
    }

    static SymbolContainer * instance() {
        static thread_local int callDepth = 0;
        static thread_local bool inRecursiveCall = false;
        
        // Prevent infinite recursion during function calls
        if (inRecursiveCall) {
            throw std::runtime_error("Infinite recursion detected in SymbolContainer::instance()");
        }
        
        callDepth++;
        if (callDepth > 100) {  // Reasonable limit for nested calls
            throw std::runtime_error("SymbolContainer instance call depth exceeded - infinite recursion detected");
        }
        
        inRecursiveCall = true;
        
        if (!is_initialized_for_singleton_) {
            inRecursiveCall = false;
            callDepth--;
            throw std::runtime_error(
                "SymbolContainer has not been initialized. Call SymbolContainer::initialize() with the top-level "
                "script/file name first.");
        }
        
        // Meyer's Singleton: instance_ is constructed on first call using the initialized name
        static SymbolContainer instance_(initial_scope_name_for_singleton_);
        
        inRecursiveCall = false;
        callDepth--;
        return &instance_;
    }

    // default scope names
    constexpr static const std::string SCOPE_SEPARATOR         = "::";
    constexpr static const std::string DEFAULT_VARIABLES_SCOPE = SCOPE_SEPARATOR + "variables";
    constexpr static const std::string DEFAULT_CONSTANTS_SCOPE = "constants";
    constexpr static const std::string DEFAULT_FUNCTIONS_SCOPE = SCOPE_SEPARATOR + "functions";
    constexpr static const std::string DEFAULT_OTHERS_SCOPE    = SCOPE_SEPARATOR + "others";
    constexpr static const std::string METHOD_SCOPE            = SCOPE_SEPARATOR + "methods";

    // other scope names
    constexpr static const std::string CALL_SCOPE = SCOPE_SEPARATOR + "call_";

    // --- Scope management ---

    /**
     * @brief Create a new scope and enter it.
     * @param name Name of the new scope.
     */
    void create(const std::string & name);

    /**
     * @brief Enter an existing scope.
     * @param name Name of the scope to enter.
     */
    void enter(const std::string & name);

    /**
     * @brief Exit the current scope, returning to the previous one.
     */
    void enterPreviousScope();

    /**
     * @brief Validate and cleanup scope stack integrity after corruption is detected
     * @param expectedScope The scope that should have been at the top of the stack
     */
    void validateAndCleanupScopeStack(const std::string & expectedScope);

    /**
     * @brief Enhanced enterPreviousScope with validation
     */
    bool enterPreviousScopeWithValidation(const std::string & expectedCurrentScope = "");

    /**
     * @brief Get the name of the current scope.
     * @return Current scope name.
     */
    [[nodiscard]] std::string currentScopeName() const;

    std::vector<std::string> getScopeNames() const;

    /**
     * @brief Get the current scope stack (from most global to most local).
     * @return Vector of scope names in stack order.
     */
    [[nodiscard]] const std::vector<std::string>& getScopeStack() const;

    // --- Symbol operations ---

    /**
     * @brief Enters a new, unique scope for a function call.
     * This scope is used for local variables of that specific call.
     * @param baseFunctionScopeName The definition scope name of the function being called.
     * @return The name of the newly created unique call scope.
     */
    std::string enterFunctionCallScope(const std::string & baseFunctionScopeName);

    /**
     * @brief Add a symbol to the current scope.
     * @param symbol Symbol to add.
     * @return Namespace under which the symbol was defined.
     */
    std::string add(const SymbolPtr & symbol);

    /**
     * @brief Add a function to the appropriate scope
     * @param function The function symbol to add
     * @param scopeName Optional scope name to define function in (defaults to current scope)
     * @return Namespace under which the function was defined
     */
    std::string addFunction(const SymbolPtr & function);

    /**
     * @brief Add a function to a specific scope
     * @param function The function symbol to add
     * @param scopeName The name of the scope to define the function in
     * @return Namespace under which the function was defined
     */
    std::string addFunction(const SymbolPtr & function, const std::string & scopeName);

    /**
     * @brief Add a method to the current scope
     * @param method The method symbol to add
     * @return Namespace under which the method was defined
     */
    std::string addMethod(const SymbolPtr & method);

    /**
     * @brief Add a method to a specific scope
     * @param method The method symbol to add
     * @param scopeName The name of the scope to define the method in
     * @return Namespace under which the method was defined
     */
    std::string addMethod(const SymbolPtr & method, const std::string & scopeName);

    /**
     * @brief Add a variable to the current scope
     * @param variable The variable symbol to add
     * @return Namespace under which the variable was defined
     */
    std::string addVariable(const SymbolPtr & variable);

    /**
     * @brief Add a variable to a specific scope
     * @param variable The variable symbol to add
     * @param scopeName The name of the scope to define the variable in
     * @return Namespace under which the variable was defined
     */
    std::string addVariable(const SymbolPtr & variable, const std::string & scopeName);

    /**
     * @brief Add a constant to the current scope
     * @param constant The constant symbol to add
     * @return Namespace under which the constant was defined
     */
    std::string addConstant(const SymbolPtr & constant);

    /**
     * @brief Add a constant to a specific scope
     * @param constant The constant symbol to add
     * @param scopeName The name of the scope to define the constant in
     * @return Namespace under which the constant was defined
     */
    std::string addConstant(const SymbolPtr & constant, const std::string & scopeName);

    /**
     * @brief Add a class definition to the current scope
     * @param classSymbol The class symbol to add
     * @return Namespace under which the class was defined
     */
    std::string addClass(const SymbolPtr & classSymbol);

    /**
     * @brief Add a class definition to a specific scope
     * @param classSymbol The class symbol to add
     * @param scopeName The name of the scope to define the class in
     * @return Namespace under which the class was defined
     */
    std::string addClass(const SymbolPtr & classSymbol, const std::string & scopeName);

    /**
     * @brief Add an enum to the current scope
     * @param enumSymbol The enum symbol to add
     * @return Namespace under which the enum was defined
     */
    std::string addEnum(const SymbolPtr & enumSymbol);

    /**
     * @brief Add an enum to a specific scope
     * @param enumSymbol The enum symbol to add
     * @param scopeName The name of the scope to define the enum in
     * @return Namespace under which the enum was defined
     */
    std::string addEnum(const SymbolPtr & enumSymbol, const std::string & scopeName);

    std::vector<SymbolPtr> getAll(const std::string & ns = "") const {
        std::vector<SymbolPtr> result;
        for (const auto & [_, table] : scopes_) {
            auto symbols = ns.empty() ? table->listAll() : table->listAll(ns);
            result.insert(result.end(), symbols.begin(), symbols.end());
        }
        return result;
    }

    /**
     * @brief Check if a symbol exists in the given namespace (or current scope if none provided).
     * @param name Symbol name.
     * @param fullNamespace Namespace to search within (defaults to current scope).
     * @return True if the symbol exists, false otherwise.
     */
    bool exists(const std::string & name, std::string fullNamespace = "") const;

    SymbolPtr get(const std::string & fullNamespace, const std::string & name) const;

    /**
     * @brief Find a symbol by name, searching hierarchically from current scope upwards.
     * Checks variables, constants, functions, and methods sub-namespaces within each scope.
     * @param name The name of the symbol to find.
     * @return Shared pointer to the found symbol, or nullptr if not found.
     */
    SymbolPtr findSymbol(const std::string & name);

    /**
     * @brief Find the namespace in which a class is defined.
     * @param className Name of the class to find.
     * @return The namespace containing the class, or empty string if not found.
     */
    std::string findClassNamespace(const std::string & className) {
        // Look in all scopes for a symbol with metadata indicating it's the class definition
        for (const auto & [scopeName, table] : scopes_) {
            // Classes are registered in the variables namespace (as per addClass method)
            auto classSymbol = table->get(DEFAULT_VARIABLES_SCOPE, className);
            if (classSymbol && classSymbol->getKind() == Kind::Class) {
                // Found the class definition
                return scopeName;
            }
        }
        return "";
    }

    /**
     * @brief Find a method within a class scope
     * @param className The name of the class
     * @param methodName The name of the method to find
     * @return Shared pointer to the found method symbol, or nullptr if not found
     */
    SymbolPtr findMethod(const std::string & className, const std::string & methodName) {
        // First check the class registry (where native methods are stored)
        if (hasClass(className)) {
            const ClassInfo & classInfo = getClassInfo(className);
            
            for (const auto & method : classInfo.methods) {
                if (method.name == methodName) {
                    // For native methods, we need to indicate that the method exists but the actual call
                    // will go through callMethod() which handles native methods directly.
                    // Since MethodCallExpressionNode only checks if the return is nullptr or not,
                    // we need to return a non-null pointer. Let's return a pointer to a static value.
                    static SymbolPtr dummySymbol = nullptr;
                    if (!dummySymbol) {
                        dummySymbol = std::make_shared<Symbols::VariableSymbol>(
                            "dummy",
                            Symbols::ValuePtr::null(),
                            "",
                            Variables::Type::NULL_TYPE
                        );
                    }
                    return dummySymbol;
                }
            }
        }
        
        // If not found in class registry, try scope-based lookup (for script-defined methods)
        std::string classScope = findClassNamespace(className);
        if (classScope.empty()) {
            return nullptr;  // Class not found
        }

        // The method is stored in a class-specific namespace: classScope::className
        std::string classMethodScope = classScope + SCOPE_SEPARATOR + className;

        // Get the symbol table for the class-specific method scope
        auto scopeTable = getScopeTable(classMethodScope);
        if (!scopeTable) {
            return nullptr;
        }

        // Look for the method in the methods namespace within the class-specific scope
        auto result = scopeTable->get(METHOD_SCOPE, methodName);
        return result;
    }

    /**
     * @brief Get method parameters for a native method
     * @param className The name of the class
     * @param methodName The name of the method
     * @return Vector of parameter info for the method, empty if not found
     */
    std::vector<Symbols::FunctionParameterInfo> getNativeMethodParameters(const std::string & className, const std::string & methodName) {
        if (hasClass(className)) {
            const ClassInfo & classInfo = getClassInfo(className);
            
            for (const auto & method : classInfo.methods) {
                if (method.name == methodName) {
                    return method.parameters;
                }
            }
        }
        return {}; // Return empty vector if not found
    }

    /**
     * @brief Get a function from the current scope or parent scopes
     * @param name The name of the function to retrieve
     * @return Shared pointer to the found function, or nullptr if not found
     */
    SymbolPtr getFunction(const std::string & name) const;

    /**
     * @brief Get a function from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the function to retrieve
     * @return Shared pointer to the found function, or nullptr if not found
     */
    SymbolPtr getFunction(const std::string & scopeName, const std::string & name) const;

    /**
     * @brief Get a method from the current scope or parent scopes
     * @param name The name of the method to retrieve
     * @return Shared pointer to the found method, or nullptr if not found
     */
    SymbolPtr getMethod(const std::string & name) const;

    /**
     * @brief Get a method from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the method to retrieve
     * @return Shared pointer to the found method, or nullptr if not found
     */
    SymbolPtr getMethod(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto method = it->second->get(METHOD_SCOPE, name);
        if (method && method->getKind() == Kind::Function) {
            return method;
        }
        return nullptr;
    }

    /**
     * @brief Get a variable from the current scope or parent scopes
     * @param name The name of the variable to retrieve
     * @return Shared pointer to the found variable, or nullptr if not found
     */
    SymbolPtr getVariable(const std::string & name) const {
        // Search scopes in innermost-to-outermost order
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto                tableIt   = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto variable = tableIt->second->get(DEFAULT_VARIABLES_SCOPE, name);
                if (variable && variable->getKind() == Kind::Variable) {
                    return variable;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Get a variable from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the variable to retrieve
     * @return Shared pointer to the found variable, or nullptr if not found
     */
    SymbolPtr getVariable(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto variable = it->second->get(DEFAULT_VARIABLES_SCOPE, name);
        if (variable) {
            if (variable->getKind() == Kind::Variable) {
                return variable;
            }
        }
        return nullptr;
    }

    /**
     * @brief Get a constant from the current scope or parent scopes
     * @param name The name of the constant to retrieve
     * @return Shared pointer to the found constant, or nullptr if not found
     */
    SymbolPtr getConstant(const std::string & name) const {
        // Search scopes in innermost-to-outermost order
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto                tableIt   = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto constant = tableIt->second->get(DEFAULT_CONSTANTS_SCOPE, name);
                if (constant && constant->getKind() == Kind::Constant) {
                    return constant;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Get a constant from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the constant to retrieve
     * @return Shared pointer to the found constant, or nullptr if not found
     */
    SymbolPtr getConstant(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto constant = it->second->get(DEFAULT_CONSTANTS_SCOPE, name);
        if (constant && constant->getKind() == Kind::Constant) {
            return constant;
        }
        return nullptr;
    }

    /**
     * @brief Get an enum from the current scope or parent scopes
     * @param name The name of the enum to retrieve
     * @return Shared pointer to the found enum, or nullptr if not found
     */
    SymbolPtr getEnum(const std::string & name) const {
        // Search scopes in innermost-to-outermost order
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto                tableIt   = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto enumSymbol = tableIt->second->get(DEFAULT_VARIABLES_SCOPE, name);
                if (enumSymbol && enumSymbol->getKind() == Kind::ENUM) {
                    return enumSymbol;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Get an enum from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the enum to retrieve
     * @return Shared pointer to the found enum, or nullptr if not found
     */
    SymbolPtr getEnum(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto enumSymbol = it->second->get(DEFAULT_VARIABLES_SCOPE, name);
        if (enumSymbol && enumSymbol->getKind() == Kind::ENUM) {
            return enumSymbol;
        }
        return nullptr;
    }

    static std::string dump();

    /** @brief Get the SymbolTable for a specific scope name, if it exists. */
    std::shared_ptr<SymbolTable> getScopeTable(const std::string & scopeName) const;

    // --- Class Registry Methods ---

    /**
     * @brief Register a new class
     * @param className Name of the class to register
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     */
    ClassInfo & registerClass(const std::string & className, Modules::BaseModule * module = nullptr) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }

        ClassInfo classInfo;
        classInfo.name   = className;
        classInfo.module = module;

        classes_[className] = classInfo;
        return classes_[className];
    }

    /**
     * @brief Register a new class with inheritance
     * @param className Name of the class to register
     * @param parentClassName Name of the parent class
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     */
    ClassInfo & registerClass(const std::string & className, const std::string & parentClassName,
                              Modules::BaseModule * module = nullptr) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }

        if (!hasClass(parentClassName)) {
            throw std::runtime_error("Parent class not found: " + parentClassName);
        }

        ClassInfo classInfo;
        classInfo.name        = className;
        classInfo.parentClass = parentClassName;
        classInfo.module      = module;

        classes_[className] = classInfo;
        return classes_[className];
    }

    /**
     * @brief Check if a class is registered
     * @param className Name of the class to check
     * @return True if the class is registered, false otherwise
     */
    bool hasClass(const std::string & className) const;

    /**
     * @brief Get information about a registered class
     * @param className Name of the class to get information for
     * @return Reference to the ClassInfo for the class
     */
    ClassInfo & getClassInfo(const std::string & className);

    /**
     * @brief Get information about a registered class (const version)
     * @param className Name of the class to get information for
     * @return Const reference to the ClassInfo for the class
     */
    const ClassInfo & getClassInfo(const std::string & className) const;

    /**
     * @brief Add a property to a class
     * @param className Name of the class to add the property to
     * @param propertyName Name of the property
     * @param type Type of the property
     * @param isPrivate Whether the property is private
     * @param defaultValueExpr Expression for the default value (optional)
     */
    void addProperty(const std::string & className, const std::string & propertyName, Variables::Type type,
                     bool isPrivate = false, Parser::ParsedExpressionPtr defaultValueExpr = nullptr);

    /**
     * @brief Add a method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     */
    void addMethod(const std::string & className, const std::string & methodName,
                   Variables::Type                            returnType = Variables::Type::NULL_TYPE,
                   std::vector<FunctionParameterInfo> parameters = {}, bool isPrivate = false);

    /**
     * @brief Add a native method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param implementation Native implementation of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     */
    void addNativeMethod(const std::string & className, const std::string & methodName,
                         std::function<ValuePtr(const std::vector<ValuePtr> &)> implementation,
                         Variables::Type                                        returnType = Variables::Type::NULL_TYPE,
                         std::vector<FunctionParameterInfo> parameters = {}, bool isPrivate = false,
                         const std::string & description = "");

    /**
     * @brief Check if a class has a specific property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasProperty(const std::string & className, const std::string & propertyName) const;

    /**
     * @brief Check if a class has a specific method
     * @param className Name of the class to check
     * @param methodName Name of the method to check for
     * @return True if the method exists, false otherwise
     */
    bool hasMethod(const std::string & className, const std::string & methodName) const;

private:
    /**
     * @brief Internal recursive method with cycle detection
     */
    bool hasMethodInternal(const std::string & className, const std::string & methodName,
                          std::unordered_set<std::string>& visited, int depth) const {
        const int MAX_RECURSION_DEPTH = 10;
        
        // Check for infinite recursion
        if (depth > MAX_RECURSION_DEPTH) {
            return false;
        }
        
        // Check for circular inheritance
        if (visited.find(className) != visited.end()) {
            return false;
        }
        
        visited.insert(className);
        
        if (!hasClass(className)) {
            return false;
        }

        const ClassInfo & classInfo = getClassInfo(className);

        // Check in this class
        for (const auto & method : classInfo.methods) {
            if (method.name == methodName) {
                return true;
            }
        }

        // Check in parent class if exists
        if (!classInfo.parentClass.empty()) {
            return hasMethodInternal(classInfo.parentClass, methodName, visited, depth + 1);
        }

        return false;
    }

public:
    /**
     * @brief Get all registered class names
     * @return Vector of class names
     */
    std::vector<std::string> getClassNames() const;

    /**
     * @brief Get namespace for a symbol based on its kind
     * @param symbol The symbol to get namespace for
     * @return The appropriate namespace string
     */
    std::string getNamespaceForSymbol(const SymbolPtr & symbol) const;

    /**
     * @brief Recursively dump value contents for debugging
     * @param value The value to dump
     * @param result String to append dump to
     * @param indent Current indentation level
     */
    static void dumpValue(const ValuePtr & value, std::string & result, int indent = 0);

    // --- Module Management Methods ---

    /**
     * @brief Get all module names
     * @return Vector of all loaded module names
     */
    std::vector<std::string> getModuleNames() const;

    /**
     * @brief Get built-in module names
     * @return Vector of built-in module names
     */
    std::vector<std::string> getBuiltInModuleNames() const;

    /**
     * @brief Get external module names
     * @return Vector of external module names
     */
    std::vector<std::string> getExternalModuleNames() const;

    /**
     * @brief Get description for a module
     * @param moduleName Name of the module
     * @return Description string, or empty if not found
     */
    std::string getModuleDescription(const std::string & moduleName) const;

    /**
     * @brief Register a module and its functions
     * @param module Unique pointer to the BaseModule
     */
    void registerModule(Modules::BaseModulePtr module);

    /**
     * @brief Store a module after registration
     * @param module Unique pointer to the BaseModule
     */
    void storeModule(Modules::BaseModulePtr module);

    /**
     * @brief Set the current module for registration context
     * @param module Pointer to the current module
     */
    void setCurrentModule(Modules::BaseModule * module);

    /**
     * @brief Get the current module
     * @return Pointer to current module, or nullptr
     */
    Modules::BaseModule * getCurrentModule() const;

    /**
     * @brief Check if a module is loaded
     * @param moduleName Name of the module to check
     * @return True if loaded, false otherwise
     */
    bool hasModule(const std::string & moduleName) const;

    /**
     * @brief Get a module by name
     * @param moduleName Name of the module
     * @return Pointer to the module if found, else nullptr
     */
    Modules::BaseModule * getModule(const std::string & moduleName) const;

    // --- Function Management Methods ---

    /**
     * @brief Register documentation for a function or method
     * @param name Function/method name
     * @param doc Documentation structure
     */
    void registerDoc(const std::string & name, const FunctionDoc & doc);

    /**
     * @brief Register a function with callback
     * @param name Function name
     * @param callback Function callback
     * @param returnType Return type
     * @param module Module that defines this function (optional)
     */
    void registerFunction(const std::string & name, CallbackFunction callback,
                         Variables::Type returnType = Variables::Type::NULL_TYPE,
                         Modules::BaseModule * module = nullptr);

    /**
     * @brief Register a function with documentation
     * @param name Function name
     * @param doc Function documentation
     * @param parameters Function parameters
     * @param plainbody Plain body (for native functions)
     * @param returnType Return type
     */
    void registerFunction(const std::string & name, const FunctionDoc & doc,
                         const std::vector<FunctionParameterInfo> & parameters,
                         const std::string & plainbody, Variables::Type returnType);

    /**
     * @brief Check if a function is registered
     * @param name Function name
     * @return True if function exists, false otherwise
     */
    bool hasFunction(const std::string & name) const;

    /**
     * @brief Call a registered function
     * @param name Function name
     * @param args Function arguments
     * @return Function result
     */
    ValuePtr callFunction(const std::string & name, const std::vector<ValuePtr> & args);

    /**
     * @brief Call a method on a class
     * @param className Class name
     * @param methodName Method name
     * @param args Method arguments
     * @return Method result
     */
    ValuePtr callMethod(const std::string & className, const std::string & methodName,
                       const std::vector<ValuePtr> & args);

    /**
     * @brief Get method parameters for a class method
     * @param className Class name
     * @param methodName Method name
     * @return Vector of parameter info
     */
    std::vector<FunctionParameterInfo> getMethodParameters(const std::string & className,
                                                          const std::string & methodName) const;

    /**
     * @brief Get function documentation
     * @param name Function name
     * @return Function documentation
     */
    const FunctionDoc & getFunctionDoc(const std::string & name) const;

    /**
     * @brief Get function return type
     * @param name Function name
     * @return Return type
     */
    Variables::Type getFunctionReturnType(const std::string & name) const;

    /**
     * @brief Get method return type
     * @param className Class name
     * @param methodName Method name
     * @return Return type
     */
    Variables::Type getMethodReturnType(const std::string & className, const std::string & methodName) const;

    /**
     * @brief Get all function names belonging to a specific module
     * @param module The module to get functions for
     * @return Vector of function names
     */
    std::vector<std::string> getFunctionNamesByModule(const Modules::BaseModule * module) const;

    /**
     * @brief Get the module that defines a specific class
     * @param className The name of the class
     * @return Pointer to the module, or nullptr if not found
     */
    Modules::BaseModule * getClassModule(const std::string & className) const;

    /**
     * @brief Get all method names for a specific class
     * @param className The name of the class
     * @return Vector of method names
     */
    std::vector<std::string> getMethodNames(const std::string & className) const;

    /**
     * @brief Get the module that defines a specific function
     * @param functionName The name of the function
     * @return Pointer to the module, or nullptr if not found
     */
    Modules::BaseModule * getFunctionModule(const std::string & functionName) const;

    /**
     * @brief Check if a method is private
     * @param className The name of the class
     * @param methodName The name of the method
     * @return True if the method is private, false otherwise
     */
    bool isMethodPrivate(const std::string & className, const std::string & methodName) const;

    /**
     * @brief Check if a property is private
     * @param className The name of the class
     * @param propertyName The name of the property
     * @return True if the property is private, false otherwise
     */
    bool isPropertyPrivate(const std::string & className, const std::string & propertyName) const;

    /**
     * @brief Get an object property
     * @param className The name of the class
     * @param propertyName The name of the property
     * @return Value of the property, or null if not found
     */
    ValuePtr getObjectProperty(const std::string & className, const std::string & propertyName) const;

    /**
     * @brief Set an object property
     * @param className The name of the class
     * @param propertyName The name of the property
     * @param value The value to set
     */
    void setObjectProperty(const std::string & className, const std::string & propertyName, const ValuePtr & value);
};

}  // namespace Symbols

#endif  // SYMBOL_CONTAINER_HPP
