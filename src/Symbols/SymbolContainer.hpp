#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include <atomic>  // Required for std::atomic
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "SymbolTable.hpp"

namespace Symbols {

class SymbolContainer {
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> scopes_;
    // Stack of active scope names (supports nested scope entry)
    std::vector<std::string>                                      scopeStack_;

    // For unique call frame IDs
    inline static std::atomic<unsigned long long> next_call_frame_id_ = 0;

    // For singleton initialization
    static std::string initial_scope_name_for_singleton_;
    static bool        is_initialized_for_singleton_;

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

    // other scope names
    // TODO: add for_ and while_ too
    constexpr static const std::string CALL_SCOPE = SCOPE_SEPARATOR + "call_";

    // The following explicit constructor is now replaced by the private one and initialize() pattern
    // explicit SymbolContainer(const std::string & default_scope_name) { create(default_scope_name); }

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
        const std::string ns = getNamespaceForSymbol(symbol);
        scopes_[currentScopeName()]->define(ns, symbol);
        return ns;
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
        const std::string ns = getNamespaceForSymbol(symbol);
        it->second->define(ns, symbol);  // Define in the found scope's table
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
     * Checks variables and constants sub-namespaces within each scope.
     * @param name The name of the symbol to find.
     * @return Shared pointer to the found symbol, or nullptr if not found.
     */
    SymbolPtr findSymbol(const std::string & name) {
        // Helper to check if a scope is a loop scope
        auto isLoopScope = [](const std::string & scope) {
            return scope.find("for_") != std::string::npos || scope.find("while_") != std::string::npos;
        };

        // Search through all scopes from innermost to outermost
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scope_name = *it;
            auto                table_it   = scopes_.find(scope_name);
            if (table_it == scopes_.end()) {
                continue;
            }

            // Check variables namespace first
            SymbolPtr symbol = table_it->second->get(DEFAULT_VARIABLES_SCOPE, name);
            if (symbol) {
                return symbol;
            }

            // Check constants namespace next
            symbol = table_it->second->get(DEFAULT_CONSTANTS_SCOPE, name);
            if (symbol) {
                return symbol;
            }

            // If we're in any loop scope, continue searching parent scopes
            if (isLoopScope(scope_name)) {
                continue;
            }

            // For non-loop scopes, stop at the first scope that doesn't have the symbol
            break;
        }

        // If not found in any scope
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

  private:

    static void dumpValue(const Value::ValuePtr & value, std::string & result, int indent) {
        using ObjectMap = Value::ObjectMap;
        if (value->getType() == Variables::Type::OBJECT) {
            const auto & objMap = std::get<ObjectMap>(value->get());
            for (const auto & [key, childVal] : objMap) {
                result += std::string(indent, '\t') + "- " + key + ": '" + Symbols::Value::to_string(childVal) + "'\n";
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
};

}  // namespace Symbols

#endif
