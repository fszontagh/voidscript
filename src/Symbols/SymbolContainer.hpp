#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include <atomic>  // Required for std::atomic
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <functional>

#include "SymbolTable.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/FunctionParameterInfo.hpp"

// Forward declarations to avoid circular dependencies
namespace Parser {
    class ParsedExpression;
    using ParsedExpressionPtr = std::shared_ptr<ParsedExpression>;
}

namespace Modules {
    class BaseModule;
}

namespace Symbols {

/**
 * @brief Documentation structure for functions and methods.
 */
struct FunctionDoc {
    std::string                     name;           // function/method name
    Variables::Type                 returnType = Variables::Type::NULL_TYPE;     // return type
    std::vector<FunctionParameterInfo> parameterList;  // parameter list
    std::string                     description;    // short description
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
    std::string                 name;
    std::string                 qualifiedName;
    Variables::Type             returnType;
    std::vector<FunctionParameterInfo> parameters;
    bool                        isPrivate = false;
    std::function<ValuePtr(const std::vector<ValuePtr>&)> nativeImplementation;
    FunctionDoc                 documentation;
};

/**
 * @brief Information about a class
 */
struct ClassInfo {
    std::string                 name;
    std::string                 parentClass;  // For inheritance
    std::vector<PropertyInfo>   properties;
    std::vector<MethodInfo>     methods;
    std::unordered_map<std::string, ValuePtr> staticProperties;
    Modules::BaseModule*        module = nullptr;  // Module that defined this class
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
    std::unordered_map<std::string, FunctionDoc> functionDocs_;
    
    // Current module being registered (for macro support)
    Modules::BaseModule* currentModule_ = nullptr;

    // Private constructor
    explicit SymbolContainer(const std::string & default_scope_name) {
        if (default_scope_name.empty()) {
            throw std::runtime_error("SymbolContainer default scope name cannot be empty during construction.");
        }
        create(default_scope_name);  // Creates and enters the initial scope
    }

  public:
    SymbolContainer(const SymbolContainer &)             = delete;
    SymbolContainer & operator=(const SymbolContainer &) = delete;

    static void initialize(const std::string & initial_scope_name) {
        if (initial_scope_name.empty()) {
            throw std::invalid_argument("Initial scope name for SymbolContainer cannot be empty.");
        }
        if (is_initialized_for_singleton_) {
            // Log re-initialization, but allow it for now. Could be an error in stricter scenarios.
            std::cerr << "Warning: SymbolContainer already initialized. Re-initializing with scope: "
                      << initial_scope_name << '\n';
        }
        initial_scope_name_for_singleton_ = initial_scope_name;
        is_initialized_for_singleton_     = true;
    }

    static SymbolContainer * instance() {
        if (!is_initialized_for_singleton_) {
            // It's crucial that initialize() is called before the first instance() call.
            // This typically happens at application startup.
            throw std::runtime_error(
                "SymbolContainer has not been initialized. Call SymbolContainer::initialize() with the top-level "
                "script/file name first.");
        }
        // Meyer's Singleton: instance_ is constructed on first call using the initialized name
        static SymbolContainer instance_(initial_scope_name_for_singleton_);
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
    void create(const std::string & name) {
        scopes_[name] = std::make_shared<SymbolTable>(SCOPE_SEPARATOR);
        scopeStack_.push_back(name);
    }

    /**
     * @brief Enter an existing scope.
     * @param name Name of the scope to enter.
     */
    void enter(const std::string & name) {
        auto it = scopes_.find(name);
        if (it != scopes_.end()) {
            scopeStack_.push_back(name);
        } else {
            throw std::runtime_error("Scope does not exist: " + name);
        }
    }

    /**
     * @brief Exit the current scope, returning to the previous one.
     */
    void enterPreviousScope() {
        if (scopeStack_.size() > 1) {
            scopeStack_.pop_back();
        }
    }

    /**
     * @brief Get the name of the current scope.
     * @return Current scope name.
     */
    [[nodiscard]] std::string currentScopeName() const {
        return scopeStack_.empty() ? std::string() : scopeStack_.back();
    }

    std::vector<std::string> getScopeNames() const {
        std::vector<std::string> result;
        result.reserve(scopes_.size());
        for (const auto & [scopeName, _] : scopes_) {
            result.push_back(scopeName);
        }
        return result;
    }

    // --- Symbol operations ---

    /**
     * @brief Enters a new, unique scope for a function call.
     * This scope is used for local variables of that specific call.
     * @param baseFunctionScopeName The definition scope name of the function being called.
     * @return The name of the newly created unique call scope.
     */
    std::string enterFunctionCallScope(const std::string & baseFunctionScopeName) {
        unsigned long long call_id = next_call_frame_id_++;
        std::string        callScopeName =
            baseFunctionScopeName + Symbols::SymbolContainer::CALL_SCOPE + std::to_string(call_id);
        create(callScopeName);  // create() also enters the scope by pushing to scopeStack_
        return callScopeName;
    }

    /**
     * @brief Add a symbol to the current scope.
     * @param symbol Symbol to add.
     * @return Namespace under which the symbol was defined.
     */
    std::string add(const SymbolPtr & symbol) {
        switch (symbol->getKind()) {
            case Symbols::Kind::Variable:
                return addVariable(symbol);
            case Symbols::Kind::Function:
                return addFunction(symbol);
            case Symbols::Kind::Method:
                return addMethod(symbol);
            case Symbols::Kind::Class:
                return addClass(symbol);
            case Symbols::Kind::Constant:
                return addConstant(symbol);
            default:
                // Fall back to generic handling
                const std::string ns = getNamespaceForSymbol(symbol);
                scopes_[currentScopeName()]->define(ns, symbol);
                return ns;
        }
    }

    /**
     * @brief Add a function to the appropriate scope
     * @param function The function symbol to add
     * @param scopeName Optional scope name to define function in (defaults to current scope)
     * @return Namespace under which the function was defined
     */
    std::string addFunction(const SymbolPtr & function) {
        if (function->getKind() != Symbols::Kind::Function) {
            throw std::runtime_error("Symbol must be a function to use addFunction");
        }
        const std::string ns = DEFAULT_FUNCTIONS_SCOPE;
        scopes_[currentScopeName()]->define(ns, function);
        return ns;
    }

    /**
     * @brief Add a function to a specific scope
     * @param function The function symbol to add
     * @param scopeName The name of the scope to define the function in
     * @return Namespace under which the function was defined
     */
    std::string addFunction(const SymbolPtr & function, const std::string & scopeName) {
        if (function->getKind() != Symbols::Kind::Function) {
            throw std::runtime_error("Symbol must be a function to use addFunction");
        }
        
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define function in non-existent scope: " + scopeName);
        }
        
        const std::string ns = DEFAULT_FUNCTIONS_SCOPE;
        it->second->define(ns, function);
        return ns;
    }

    /**
     * @brief Add a method to the current scope
     * @param method The method symbol to add
     * @return Namespace under which the method was defined
     */
    std::string addMethod(const SymbolPtr & method) {
        if (method->getKind() != Symbols::Kind::Function && method->getKind() != Symbols::Kind::Method) {
            throw std::runtime_error("Symbol must be a function or method to use addMethod");
        }
        const std::string ns = METHOD_SCOPE;
        scopes_[currentScopeName()]->define(ns, method);
        return ns;
    }

    /**
     * @brief Add a method to a specific scope
     * @param method The method symbol to add
     * @param scopeName The name of the scope to define the method in
     * @return Namespace under which the method was defined
     */
    std::string addMethod(const SymbolPtr & method, const std::string & scopeName) {
        if (method->getKind() != Symbols::Kind::Function && method->getKind() != Symbols::Kind::Method) {
            throw std::runtime_error("Symbol must be a function or method to use addMethod");
        }
        
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define method in non-existent scope: " + scopeName);
        }
        
        const std::string ns = METHOD_SCOPE;
        it->second->define(ns, method);
        return ns;
    }

    /**
     * @brief Add a variable to the current scope
     * @param variable The variable symbol to add
     * @return Namespace under which the variable was defined
     */
    std::string addVariable(const SymbolPtr & variable) {
        if (variable->getKind() != Symbols::Kind::Variable) {
            throw std::runtime_error("Symbol must be a variable to use addVariable");
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[currentScopeName()]->define(ns, variable);
        return ns;
    }

    /**
     * @brief Add a variable to a specific scope
     * @param variable The variable symbol to add
     * @param scopeName The name of the scope to define the variable in
     * @return Namespace under which the variable was defined
     */
    std::string addVariable(const SymbolPtr & variable, const std::string & scopeName) {
        if (variable->getKind() != Symbols::Kind::Variable) {
            throw std::runtime_error("Symbol must be a variable to use addVariable");
        }
        
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define variable in non-existent scope: " + scopeName);
        }
        
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, variable);
        return ns;
    }

    /**
     * @brief Add a constant to the current scope
     * @param constant The constant symbol to add
     * @return Namespace under which the constant was defined
     */
    std::string addConstant(const SymbolPtr & constant) {
        if (constant->getKind() != Symbols::Kind::Constant) {
            throw std::runtime_error("Symbol must be a constant to use addConstant");
        }
        const std::string ns = DEFAULT_CONSTANTS_SCOPE;
        scopes_[currentScopeName()]->define(ns, constant);
        return ns;
    }

    /**
     * @brief Add a constant to a specific scope
     * @param constant The constant symbol to add
     * @param scopeName The name of the scope to define the constant in
     * @return Namespace under which the constant was defined
     */
    std::string addConstant(const SymbolPtr & constant, const std::string & scopeName) {
        if (constant->getKind() != Symbols::Kind::Constant) {
            throw std::runtime_error("Symbol must be a constant to use addConstant");
        }
        
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define constant in non-existent scope: " + scopeName);
        }
        
        const std::string ns = DEFAULT_CONSTANTS_SCOPE;
        it->second->define(ns, constant);
        return ns;
    }

    /**
     * @brief Add a class definition to the current scope
     * @param classSymbol The class symbol to add
     * @return Namespace under which the class was defined
     */
    std::string addClass(const SymbolPtr & classSymbol) {
        if (classSymbol->getKind() != Symbols::Kind::Class) {
            throw std::runtime_error("Symbol must be a class to use addClass");
        }
        // Classes are stored in the default namespace
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[currentScopeName()]->define(ns, classSymbol);
        return ns;
    }

    /**
     * @brief Add a class definition to a specific scope
     * @param classSymbol The class symbol to add
     * @param scopeName The name of the scope to define the class in
     * @return Namespace under which the class was defined
     */
    std::string addClass(const SymbolPtr & classSymbol, const std::string & scopeName) {
        if (classSymbol->getKind() != Symbols::Kind::Class) {
            throw std::runtime_error("Symbol must be a class to use addClass");
        }
        
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define class in non-existent scope: " + scopeName);
        }
        
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, classSymbol);
        return ns;
    }

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
    bool exists(const std::string & name, std::string fullNamespace = "") const {
        if (fullNamespace.empty()) {
            fullNamespace = currentScopeName();
        }

        // Search scopes in innermost-to-outermost order for shadowing
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto         tableIt   = scopes_.find(scopeName);
            if (tableIt != scopes_.end() && tableIt->second->exists(fullNamespace, name)) {
                return true;
            }
        }
        return false;
    }

    SymbolPtr get(const std::string & fullNamespace, const std::string & name) const {
        // Search scopes in innermost-to-outermost order for first matching symbol
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto         tableIt   = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto sym = tableIt->second->get(fullNamespace, name);
                if (sym) {
                    return sym;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Find a symbol by name, searching hierarchically from current scope upwards.
     * Checks variables, constants, functions, and methods sub-namespaces within each scope.
     * @param name The name of the symbol to find.
     * @return Shared pointer to the found symbol, or nullptr if not found.
     */
    SymbolPtr findSymbol(const std::string & name) {
        // Helper to check if a scope is a loop scope
        auto isLoopScope = [](const std::string & scope) {
            return scope.find("for_") != std::string::npos || scope.find("while_") != std::string::npos;
        };

        // First try the specialized getter methods
        if (SymbolPtr variable = getVariable(name)) {
            return variable;
        }
        
        if (SymbolPtr constant = getConstant(name)) {
            return constant;
        }
        
        if (SymbolPtr function = getFunction(name)) {
            return function;
        }
        
        if (SymbolPtr method = getMethod(name)) {
            return method;
        }
        
        // If we're in a loop scope, we need special handling for parent scope lookups
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scope_name = *it;
            // If we're in any loop scope, continue searching parent scopes
            if (isLoopScope(scope_name)) {
                continue;
            }
            // For non-loop scopes, we already checked with the getters above
            // and didn't find the symbol, so we can stop looking
            break;
        }

        // If not found in any scope
        return nullptr;
    }

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
        // First find the class's scope
        std::string classScope = findClassNamespace(className);
        if (classScope.empty()) {
            return nullptr; // Class not found
        }
        
        // The method is stored in a class-specific namespace: classScope::className
        std::string classMethodScope = classScope + SCOPE_SEPARATOR + className;
        
        // Get the symbol table for the class-specific method scope
        auto scopeTable = getScopeTable(classMethodScope);
        if (!scopeTable) {
            return nullptr;
        }
        
        // Look for the method in the methods namespace within the class-specific scope
        return scopeTable->get(METHOD_SCOPE, methodName);
    }

    /**
     * @brief Get a function from the current scope or parent scopes
     * @param name The name of the function to retrieve
     * @return Shared pointer to the found function, or nullptr if not found
     */
    SymbolPtr getFunction(const std::string & name) const {
        // Search scopes in innermost-to-outermost order
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto func = tableIt->second->get(DEFAULT_FUNCTIONS_SCOPE, name);
                if (func && func->getKind() == Kind::Function) {
                    return func;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Get a function from a specific scope
     * @param scopeName The name of the scope to look in
     * @param name The name of the function to retrieve
     * @return Shared pointer to the found function, or nullptr if not found
     */
    SymbolPtr getFunction(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto func = it->second->get(DEFAULT_FUNCTIONS_SCOPE, name);
        if (func && func->getKind() == Kind::Function) {
            return func;
        }
        return nullptr;
    }

    /**
     * @brief Get a method from the current scope or parent scopes
     * @param name The name of the method to retrieve
     * @return Shared pointer to the found method, or nullptr if not found
     */
    SymbolPtr getMethod(const std::string & name) const {
        // Search scopes in innermost-to-outermost order
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto method = tableIt->second->get(METHOD_SCOPE, name);
                if (method && method->getKind() == Kind::Function) {
                    return method;
                }
            }
        }
        return nullptr;
    }

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
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
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
        if (variable && variable->getKind() == Kind::Variable) {
            return variable;
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
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
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

    static std::string dump() {
        std::string result;

        result += "\n--- Symbol Table Dump ---\n";

        for (const auto & scope_name : instance()->getScopeNames()) {
            result += "Scope: '" + scope_name + "'\n";

            // Get the table for this scope
            auto tablePtr = instance()->getScopeTable(scope_name);
            if (tablePtr) {
                // Iterate all symbols in this table using the flat structure
                for (const auto & symbol : tablePtr->listAll()) {  // listAll doesn't need prefix now
                    result += symbol->dump() + '\n';
                    // Recursively dump object properties
                    dumpValue(symbol->getValue(), result, 2);  // Reduced indent slightly
                }
            } else {
                result += "\t(Error: Scope table not found)\n";
            }
        }
        result += "--- End Dump ---\n";
        return result;
    }

    /** @brief Get the SymbolTable for a specific scope name, if it exists. */
    std::shared_ptr<SymbolTable> getScopeTable(const std::string & scopeName) const {
        auto it = scopes_.find(scopeName);
        if (it != scopes_.end()) {
            return it->second;  // Return the shared_ptr to the table
        }
        return nullptr;         // Scope not found
    }

    // --- Class Registry Methods ---

    /**
     * @brief Register a new class
     * @param className Name of the class to register
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     */
    ClassInfo& registerClass(const std::string& className, Modules::BaseModule* module = nullptr) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }
        
        ClassInfo classInfo;
        classInfo.name = className;
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
    ClassInfo& registerClass(const std::string& className, const std::string& parentClassName, 
                           Modules::BaseModule* module = nullptr) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }
        
        if (!hasClass(parentClassName)) {
            throw std::runtime_error("Parent class not found: " + parentClassName);
        }
        
        ClassInfo classInfo;
        classInfo.name = className;
        classInfo.parentClass = parentClassName;
        classInfo.module = module;
        
        classes_[className] = classInfo;
        return classes_[className];
    }

    /**
     * @brief Check if a class is registered
     * @param className Name of the class to check
     * @return True if the class is registered, false otherwise
     */
    bool hasClass(const std::string& className) const {
        return classes_.find(className) != classes_.end();
    }

    /**
     * @brief Get information about a registered class
     * @param className Name of the class to get information for
     * @return Reference to the ClassInfo for the class
     */
    ClassInfo& getClassInfo(const std::string& className) {
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            throw std::runtime_error("Class not found: " + className);
        }
        return it->second;
    }

    /**
     * @brief Get information about a registered class (const version)
     * @param className Name of the class to get information for
     * @return Const reference to the ClassInfo for the class
     */
    const ClassInfo& getClassInfo(const std::string& className) const {
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            throw std::runtime_error("Class not found: " + className);
        }
        return it->second;
    }

    /**
     * @brief Add a property to a class
     * @param className Name of the class to add the property to
     * @param propertyName Name of the property
     * @param type Type of the property
     * @param isPrivate Whether the property is private
     * @param defaultValueExpr Expression for the default value (optional)
     */
    void addProperty(const std::string& className, const std::string& propertyName, 
                    Variables::Type type, bool isPrivate = false, 
                    Parser::ParsedExpressionPtr defaultValueExpr = nullptr) {
        ClassInfo& classInfo = getClassInfo(className);
        
        // Check if property already exists
        for (const auto& prop : classInfo.properties) {
            if (prop.name == propertyName) {
                throw std::runtime_error("Property already exists in class: " + className + "::" + propertyName);
            }
        }
        
        PropertyInfo propertyInfo;
        propertyInfo.name = propertyName;
        propertyInfo.type = type;
        propertyInfo.isPrivate = isPrivate;
        propertyInfo.defaultValueExpr = defaultValueExpr;
        
        classInfo.properties.push_back(propertyInfo);
    }

    /**
     * @brief Add a method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     */
    void addMethod(const std::string& className, const std::string& methodName,
                  Variables::Type returnType = Variables::Type::NULL_TYPE,
                  const std::vector<FunctionParameterInfo>& parameters = {},
                  bool isPrivate = false) {
        ClassInfo& classInfo = getClassInfo(className);
        
        // Check if method already exists
        for (const auto& method : classInfo.methods) {
            if (method.name == methodName) {
                throw std::runtime_error("Method already exists in class: " + className + "::" + methodName);
            }
        }
        
        MethodInfo methodInfo;
        methodInfo.name = methodName;
        methodInfo.qualifiedName = className + SCOPE_SEPARATOR + methodName;
        methodInfo.returnType = returnType;
        methodInfo.parameters = parameters;
        methodInfo.isPrivate = isPrivate;
        
        // Create documentation
        FunctionDoc doc;
        doc.name = methodInfo.qualifiedName;
        doc.returnType = returnType;
        doc.parameterList = parameters;
        methodInfo.documentation = doc;
        
        classInfo.methods.push_back(methodInfo);
    }

    /**
     * @brief Add a native method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param implementation Native implementation of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     */
    void addNativeMethod(const std::string& className, const std::string& methodName,
                        std::function<ValuePtr(const std::vector<ValuePtr>&)> implementation,
                        Variables::Type returnType = Variables::Type::NULL_TYPE,
                        const std::vector<FunctionParameterInfo>& parameters = {},
                        bool isPrivate = false) {
        ClassInfo& classInfo = getClassInfo(className);
        
        // Check if method already exists
        for (const auto& method : classInfo.methods) {
            if (method.name == methodName) {
                throw std::runtime_error("Method already exists in class: " + className + "::" + methodName);
            }
        }
        
        MethodInfo methodInfo;
        methodInfo.name = methodName;
        methodInfo.qualifiedName = className + SCOPE_SEPARATOR + methodName;
        methodInfo.returnType = returnType;
        methodInfo.parameters = parameters;
        methodInfo.isPrivate = isPrivate;
        methodInfo.nativeImplementation = implementation;
        
        // Create documentation
        FunctionDoc doc;
        doc.name = methodInfo.qualifiedName;
        doc.returnType = returnType;
        doc.parameterList = parameters;
        methodInfo.documentation = doc;
        
        classInfo.methods.push_back(methodInfo);
    }

    /**
     * @brief Check if a class has a specific property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasProperty(const std::string& className, const std::string& propertyName) const {
        if (!hasClass(className)) {
            return false;
        }
        
        const ClassInfo& classInfo = getClassInfo(className);
        
        // Check in this class
        for (const auto& prop : classInfo.properties) {
            if (prop.name == propertyName) {
                return true;
            }
        }
        
        // Check in parent class if exists
        if (!classInfo.parentClass.empty()) {
            return hasProperty(classInfo.parentClass, propertyName);
        }
        
        return false;
    }

    /**
     * @brief Check if a class has a specific method
     * @param className Name of the class to check
     * @param methodName Name of the method to check for
     * @return True if the method exists, false otherwise
     */
    bool hasMethod(const std::string& className, const std::string& methodName) const {
        if (!hasClass(className)) {
            return false;
        }
        
        const ClassInfo& classInfo = getClassInfo(className);
        
        // Check in this class
        for (const auto& method : classInfo.methods) {
            if (method.name == methodName) {
                return true;
            }
        }
        
        // Check in parent class if exists
        if (!classInfo.parentClass.empty()) {
            return hasMethod(classInfo.parentClass, methodName);
        }
        
        return false;
    }

    /**
     * @brief Get a list of all registered class names
     * @return Vector of registered class names
     */
    std::vector<std::string> getClassNames() const {
        std::vector<std::string> result;
        result.reserve(classes_.size());
        
        for (const auto& [className, _] : classes_) {
            result.push_back(className);
        }
        
        return result;
    }

    /**
     * @brief Set a static property value for a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @param value Value to set
     */
    void setStaticProperty(const std::string& className, const std::string& propertyName, const ValuePtr& value) {
        ClassInfo& classInfo = getClassInfo(className);
        classInfo.staticProperties[propertyName] = value;
    }

    /**
     * @brief Get a static property value from a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Value of the property
     */
    ValuePtr getStaticProperty(const std::string& className, const std::string& propertyName) const {
        const ClassInfo& classInfo = getClassInfo(className);
        
        auto it = classInfo.staticProperties.find(propertyName);
        if (it != classInfo.staticProperties.end()) {
            return it->second;
        }
        
        // Check in parent class if exists
        if (!classInfo.parentClass.empty()) {
            return getStaticProperty(classInfo.parentClass, propertyName);
        }
        
        throw std::runtime_error("Static property not found: " + className + "::" + propertyName);
    }

    /**
     * @brief Check if a class has a specific static property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasStaticProperty(const std::string& className, const std::string& propertyName) const {
        if (!hasClass(className)) {
            return false;
        }
        
        const ClassInfo& classInfo = getClassInfo(className);
        
        // Check in this class
        if (classInfo.staticProperties.find(propertyName) != classInfo.staticProperties.end()) {
            return true;
        }
        
        // Check in parent class if exists
        if (!classInfo.parentClass.empty()) {
            return hasStaticProperty(classInfo.parentClass, propertyName);
        }
        
        return false;
    }

    /**
     * @brief Get the module that defined a class
     * @param className Name of the class
     * @return Pointer to the module, or nullptr for script-defined classes
     */
    Modules::BaseModule* getClassModule(const std::string& className) const {
        const ClassInfo& classInfo = getClassInfo(className);
        return classInfo.module;
    }

    /**
     * @brief Get the current module being registered
     * @return Pointer to the current module, or nullptr if no module is being registered
     */
    Modules::BaseModule* getCurrentModule() const {
        return currentModule_;
    }

    /**
     * @brief Set the current module being registered
     * @param module Pointer to the module that is currently registering symbols
     */
    void setCurrentModule(Modules::BaseModule* module) {
        currentModule_ = module;
    }

    /**
     * @brief Set a static property for a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @param value Value to set
     */
    void setObjectProperty(const std::string& className, const std::string& propertyName, const ValuePtr& value) {
        auto& classInfo = getClassInfo(className);
        classInfo.staticProperties[propertyName] = value;
    }

    /**
     * @brief Get a static property for a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Value of the property, or nullptr if not found
     */
    ValuePtr getObjectProperty(const std::string& className, const std::string& propertyName) const {
        const auto& classInfo = getClassInfo(className);
        auto it = classInfo.staticProperties.find(propertyName);
        if (it != classInfo.staticProperties.end()) {
            return it->second;
        }
        return nullptr;
    }

    // --- Function Registry Methods ---

    /**
     * @brief Register a function
     * @param name Name of the function
     * @param callback Function implementation
     * @param returnType Return type of the function
     */
    void registerFunction(const std::string& name, CallbackFunction callback, 
                         const Variables::Type& returnType = Variables::Type::NULL_TYPE) {
        functions_[name] = callback;
        
        // Create documentation if it doesn't exist
        if (functionDocs_.find(name) == functionDocs_.end()) {
            FunctionDoc doc;
            doc.name = name;
            doc.returnType = returnType;
            functionDocs_[name] = doc;
        }
    }

    /**
     * @brief Register documentation for a function
     * @param name Name of the function
     * @param doc Documentation for the function
     */
    void registerDoc(const std::string& name, const FunctionDoc& doc) {
        functionDocs_[name] = doc;
    }

    /**
     * @brief Check if a function is registered
     * @param name Name of the function to check
     * @return True if the function is registered, false otherwise
     */
    bool hasFunction(const std::string& name) const {
        return functions_.find(name) != functions_.end();
    }

    /**
     * @brief Call a registered function
     * @param name Name of the function to call
     * @param args Arguments to pass to the function
     * @return Value returned by the function
     */
    ValuePtr callFunction(const std::string& name, FunctionArguments& args) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::runtime_error("Function not found: " + name);
        }
        
        return it->second(args);
    }

    /**
     * @brief Get the return type of a function
     * @param name Name of the function
     * @return Return type of the function
     */
    Variables::Type getFunctionReturnType(const std::string& name) const {
        auto it = functionDocs_.find(name);
        if (it == functionDocs_.end()) {
            return Variables::Type::NULL_TYPE;
        }
        
        return it->second.returnType;
    }

    /**
     * @brief Get the documentation for a function
     * @param name Name of the function
     * @return Documentation for the function
     */
    const FunctionDoc& getFunctionDoc(const std::string& name) const {
        static const FunctionDoc emptyDoc;
        
        auto it = functionDocs_.find(name);
        if (it == functionDocs_.end()) {
            return emptyDoc;
        }
        
        return it->second;
    }

    /**
     * @brief Get all function names
     * @return Vector of function names
     */
    std::vector<std::string> getFunctionNames() const {
        std::vector<std::string> names;
        for (const auto& [name, func] : functions_) {
            names.push_back(name);
        }
        return names;
    }

    /**
     * @brief Get all method names for a class
     * @param className Name of the class
     * @return Vector of method names
     */
    std::vector<std::string> getMethodNames(const std::string& className) const {
        std::vector<std::string> names;
        auto it = classes_.find(className);
        if (it != classes_.end()) {
            for (const auto& method : it->second.methods) {
                names.push_back(method.name);
            }
        }
        return names;
    }

    /**
     * @brief Get the return type of a method
     * @param className Name of the class
     * @param methodName Name of the method
     * @return Return type of the method
     */
    Variables::Type getMethodReturnType(const std::string& className, const std::string& methodName) const {
        auto it = classes_.find(className);
        if (it != classes_.end()) {
            for (const auto& method : it->second.methods) {
                if (method.name == methodName) {
                    return method.returnType;
                }
            }
        }
        return Variables::Type::NULL_TYPE;
    }

    /**
     * @brief Get the parameters of a method
     * @param className Name of the class
     * @param methodName Name of the method
     * @return Vector of method parameters
     */
    std::vector<FunctionParameterInfo> getMethodParameters(const std::string& className, const std::string& methodName) const {
        auto it = classes_.find(className);
        if (it != classes_.end()) {
            for (const auto& method : it->second.methods) {
                if (method.name == methodName) {
                    return method.parameters;
                }
            }
        }
        return {};
    }

    /**
     * @brief Call a method on a class instance
     * @param className Name of the class
     * @param methodName Name of the method
     * @param args Arguments to pass to the method
     * @return Value returned by the method
     */
    ValuePtr callMethod(const std::string& className, const std::string& methodName, FunctionArguments& args) {
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            throw std::runtime_error("Class not found: " + className);
        }
        
        for (const auto& method : it->second.methods) {
            if (method.name == methodName) {
                if (method.nativeImplementation) {
                    return method.nativeImplementation(args);
                }
                break;
            }
        }
        
        throw std::runtime_error("Method not found: " + className + "::" + methodName);
    }
  private:
    static void dumpValue(const Symbols::ValuePtr & value, std::string & result, int indent) {
        if (value == Variables::Type::OBJECT) {
            Symbols::ObjectMap objMap = value;
            for (const auto & [key, childVal] : objMap) {
                result += std::string(indent, '\t') + "- " + key + ": '" + childVal.toString() + "'\n";
                dumpValue(childVal, result, indent + 1);
            }
        }
    }

    /**
     * @brief Compute the namespace string for a symbol based on its kind and context.
     */
    static std::string getNamespaceForSymbol(const SymbolPtr & symbol) {
        switch (symbol->getKind()) {
            case Symbols::Kind::Variable:
                return DEFAULT_VARIABLES_SCOPE;
            case Symbols::Kind::Function:
                return DEFAULT_FUNCTIONS_SCOPE;
            case Symbols::Kind::Constant:
                return DEFAULT_CONSTANTS_SCOPE;
            default:
                return DEFAULT_OTHERS_SCOPE;
        }
    }

    /**
     * @brief Define a symbol in a specific scope (not necessarily the current one).
     * @param scopeName The name of the scope to define the symbol in.
     * @param symbol The symbol to define.
     * @throws std::runtime_error if the scope does not exist.
     */
    void defineInScope(const std::string & scopeName, const SymbolPtr & symbol) {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define symbol in non-existent scope: " + scopeName);
        }
        
        // Use specialized methods based on symbol type
        switch (symbol->getKind()) {
            case Symbols::Kind::Variable:
                addVariable(symbol, scopeName);
                break;
            case Symbols::Kind::Function:
                addFunction(symbol, scopeName);
                break;
            case Symbols::Kind::Constant:
                addConstant(symbol, scopeName);
                break;
            default:
                // Fall back to generic handling
                const std::string ns = getNamespaceForSymbol(symbol);
                it->second->define(ns, symbol);
                break;
        }
    }
};

}  // namespace Symbols

#endif // SYMBOL_CONTAINER_HPP