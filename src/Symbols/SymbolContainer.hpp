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
     * @brief Validate and cleanup scope stack integrity after corruption is detected
     * @param expectedScope The scope that should have been at the top of the stack
     */
    void validateAndCleanupScopeStack(const std::string & expectedScope) {
        // Find the expected scope in the stack
        auto it = std::find(scopeStack_.rbegin(), scopeStack_.rend(), expectedScope);
        if (it != scopeStack_.rend()) {
            // Found the scope, remove everything above it (including itself)
            auto forward_it = it.base() - 1; // Convert reverse iterator to forward iterator
            scopeStack_.erase(forward_it, scopeStack_.end());
        } else {
            // In severe corruption, just remove the top scope
            if (!scopeStack_.empty()) {
                scopeStack_.pop_back();
            }
        }
    }

    /**
     * @brief Enhanced enterPreviousScope with validation
     */
    bool enterPreviousScopeWithValidation(const std::string & expectedCurrentScope = "") {
        if (scopeStack_.size() <= 1) {
            return false;
        }
        
        if (!expectedCurrentScope.empty() && currentScopeName() != expectedCurrentScope) {
            return false;
        }
        
        scopeStack_.pop_back();
        return true;
    }

    /**
     * @brief Get the name of the current scope.
     * @return Current scope name.
     */
    [[nodiscard]] std::string currentScopeName() const {
        if (scopeStack_.empty()) {
            return std::string();
        }
        return scopeStack_.back();
    }

    std::vector<std::string> getScopeNames() const {
        std::vector<std::string> result;
        result.reserve(scopes_.size());
        for (const auto & [scopeName, _] : scopes_) {
            result.push_back(scopeName);
        }
        return result;
    }

    /**
     * @brief Get the current scope stack (from most global to most local).
     * @return Vector of scope names in stack order.
     */
    [[nodiscard]] const std::vector<std::string>& getScopeStack() const {
        return scopeStack_;
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
        switch (symbol->kind()) {
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
            case Symbols::Kind::ENUM:
                return addEnum(symbol);
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
        if (function->kind() != Symbols::Kind::Function) {
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
        if (function->kind() != Symbols::Kind::Function) {
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
        if (method->kind() != Symbols::Kind::Function && method->kind() != Symbols::Kind::Method) {
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
        if (method->kind() != Symbols::Kind::Function && method->kind() != Symbols::Kind::Method) {
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
        if (variable->kind() != Symbols::Kind::Variable) {
            throw std::runtime_error("Symbol must be a variable to use addVariable");
        }
        const std::string current_scope = currentScopeName();
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[current_scope]->define(ns, variable);
        return ns;
    }

    /**
     * @brief Add a variable to a specific scope
     * @param variable The variable symbol to add
     * @param scopeName The name of the scope to define the variable in
     * @return Namespace under which the variable was defined
     */
    std::string addVariable(const SymbolPtr & variable, const std::string & scopeName) {
        if (variable->kind() != Symbols::Kind::Variable) {
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
        if (constant->kind() != Symbols::Kind::Constant) {
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
        if (constant->kind() != Symbols::Kind::Constant) {
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
        if (classSymbol->kind() != Symbols::Kind::Class) {
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
        if (classSymbol->kind() != Symbols::Kind::Class) {
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

    /**
     * @brief Add an enum to the current scope
     * @param enumSymbol The enum symbol to add
     * @return Namespace under which the enum was defined
     */
    std::string addEnum(const SymbolPtr & enumSymbol) {
        if (enumSymbol->kind() != Symbols::Kind::ENUM) {
            throw std::runtime_error("Symbol must be an enum to use addEnum");
        }
        // Enums are stored in the variables namespace like classes
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[currentScopeName()]->define(ns, enumSymbol);
        return ns;
    }

    /**
     * @brief Add an enum to a specific scope
     * @param enumSymbol The enum symbol to add
     * @param scopeName The name of the scope to define the enum in
     * @return Namespace under which the enum was defined
     */
    std::string addEnum(const SymbolPtr & enumSymbol, const std::string & scopeName) {
        if (enumSymbol->kind() != Symbols::Kind::ENUM) {
            throw std::runtime_error("Symbol must be an enum to use addEnum");
        }

        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define enum in non-existent scope: " + scopeName);
        }

        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, enumSymbol);
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