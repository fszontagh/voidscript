#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "SymbolTable.hpp"

#define NSMGR Symbols::SymbolContainer::instance()

namespace Symbols {

class SymbolContainer {
    std::unordered_map<std::string, std::shared_ptr<SymbolTable>> scopes_;
    std::string                                                   currentScope_  = "global";
    std::string                                                   previousScope_ = "global";

  public:
    static SymbolContainer * instance() {
        static SymbolContainer instance_;
        return &instance_;
    }

    explicit SymbolContainer(const std::string & default_scope_name = "global") { create(default_scope_name); }

    // --- Scope management ---

    void create(const std::string & name) {
        scopes_[name]  = std::make_shared<SymbolTable>();
        previousScope_ = currentScope_;
        currentScope_  = name;
    }

    void enter(const std::string & name) {
        if (scopes_.contains(name)) {
            previousScope_ = currentScope_;
            currentScope_  = name;
        } else {
            throw std::runtime_error("Scope does not exist: " + name);
        }
    }

    void enterPreviousScope() { currentScope_ = previousScope_; }

    [[nodiscard]] std::string currentScopeName() const { return currentScope_; }

    std::vector<std::string> getScopeNames() const {
        std::vector<std::string> result;
        result.reserve(scopes_.size());
        for (const auto & [scopeName, _] : scopes_) {
            result.push_back(scopeName);
        }
        return result;
    }

    // --- Symbol operations ---

    std::string add(const SymbolPtr & symbol) {
        const std::string ns = getNamespaceForSymbol(symbol);
        scopes_[currentScope_]->define(ns, symbol);
        return ns;
    }

    std::vector<std::string> getNameSpaces(const std::string & scopeName) const {
        std::vector<std::string> result;
        auto                     it = scopes_.find(scopeName);
        if (it != scopes_.end()) {
            return it->second->listNSs();
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

    bool exists(const std::string & name, std::string fullNamespace = "") const {
        if (fullNamespace.empty()) {
            fullNamespace = currentScope_;
        }

        for (const auto & [_, table] : scopes_) {
            if (table->exists(fullNamespace, name)) {
                return true;
            }
        }
        return false;
    }

    SymbolPtr get(const std::string & fullNamespace, const std::string & name) const {
        for (const auto & [_, table] : scopes_) {
            auto sym = table->get(fullNamespace, name);
            if (sym) {
                return sym;
            }
        }
        return nullptr;
    }

    static std::string dump() {
        std::string result = "";

        std::cout << "\n--- Defined Scopes ---" << '\n';
        for (const auto & scope_name : instance()->getScopeNames()) {
            result += scope_name + '\n';
            for (const auto & sname : instance()->getNameSpaces(scope_name)) {
                result += "\t -" + sname + '\n';
                for (const auto & symbol : instance()->getAll(sname)) {
                    result += symbol->dump() + '\n';
                }
            }
        }
        return result;
    }

  private:
    std::string getNamespaceForSymbol(const SymbolPtr & symbol) const {
        std::string base = symbol->context().empty() ? currentScope_ : symbol->context();

        switch (symbol->getKind()) {
            case Symbols::Kind::Variable:
                return base + ".variables";
            case Symbols::Kind::Function:
                return base + ".functions";
            case Symbols::Kind::Constant:
                return base + ".constants";
            default:
                return base + ".others";
        }
    }
};

}  // namespace Symbols

#endif
