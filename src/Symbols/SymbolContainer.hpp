#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "SymbolTable.hpp"

#define NSMGR Symbols::SymbolContainer::instance()

namespace Symbols {

class SymbolContainer {
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> scopes_;
    // Stack of active scope names (supports nested scope entry)
    std::vector<std::string>                                        scopeStack_;

  public:
    static SymbolContainer * instance() {
        static SymbolContainer instance_;
        return &instance_;
    }

    explicit SymbolContainer(const std::string & default_scope_name = "global") { create(default_scope_name); }

    // --- Scope management ---

    /**
     * @brief Create a new scope and enter it.
     * @param name Name of the new scope.
     */
    void create(const std::string & name) {
        scopes_[name] = std::make_shared<SymbolTable>();
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
    [[nodiscard]] std::string currentScopeName() const { return scopeStack_.empty() ? std::string() : scopeStack_.back(); }

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
     * @brief Add a symbol to the current scope.
     * @param symbol Symbol to add.
     * @return Namespace under which the symbol was defined.
     */
    std::string add(const SymbolPtr & symbol) {
        const std::string ns = getNamespaceForSymbol(symbol);
        scopes_[currentScopeName()]->define(ns, symbol);
        return ns;
    }

    /** @brief List the symbol namespaces (categories) within a given scope. */
    std::vector<std::string> getNamespaces(const std::string & scopeName) const {
        std::vector<std::string> result;
        auto it = scopes_.find(scopeName);
        if (it != scopes_.end()) {
            return it->second->listNamespaces();
        }
        return result;
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
            auto tableIt = scopes_.find(scopeName);
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
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto sym = tableIt->second->get(fullNamespace, name);
                if (sym) {
                    return sym;
                }
            }
        }
        return nullptr;
    }

    static std::string dump() {
        std::string result;

        std::cout << "\n--- Defined Scopes ---" << '\n';
        for (const auto & scope_name : instance()->getScopeNames()) {
            result += scope_name + '\n';
            for (const auto & ns : instance()->getNamespaces(scope_name)) {
                result += "\t -" + ns + '\n';
                for (const auto & symbol : instance()->getAll(ns)) {
                    result += symbol->dump() + '\n';
                    // Recursively dump object properties
                    dumpValue(symbol->getValue(), result, 4);
                }
            }
        }
        return result;
    }

  private:
    // Helper to recursively dump object Value properties
    static void dumpValue(const Value &value, std::string &result, int indent) {
        using ObjectMap = Value::ObjectMap;
        if (value.getType() == Variables::Type::OBJECT) {
            const auto &objMap = std::get<ObjectMap>(value.get());
            for (const auto & [key, childVal] : objMap) {
                result += std::string(indent, '\t') + "- " + key + ": '" + Value::to_string(childVal) + "'\n";
                dumpValue(childVal, result, indent + 1);
            }
        }
    }
    /**
     * @brief Compute the namespace string for a symbol based on its kind and context.
     */
    std::string getNamespaceForSymbol(const SymbolPtr & symbol) const {
        std::string base = symbol->context().empty() ? currentScopeName() : symbol->context();

        const char *sep = "::";
        switch (symbol->getKind()) {
            case Symbols::Kind::Variable:
                return base + sep + "variables";
            case Symbols::Kind::Function:
                return base + sep + "functions";
            case Symbols::Kind::Constant:
                return base + sep + "constants";
            default:
                return base + sep + "others";
        }
    }
};

}  // namespace Symbols

#endif
