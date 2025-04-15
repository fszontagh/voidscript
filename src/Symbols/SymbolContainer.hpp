// SymbolContainer.hpp
#ifndef SYMBOL_CONTAINER_HPP
#define SYMBOL_CONTAINER_HPP

#include "SymbolTable.hpp"

namespace Symbols {

class SymbolContainer {
    std::shared_ptr<SymbolTable> globalScope_;
    std::shared_ptr<SymbolTable> currentScope_;

public:
    SymbolContainer() {
        globalScope_ = std::make_shared<SymbolTable>();
        currentScope_ = globalScope_;
    }

    void enterScope() {
        currentScope_ = std::make_shared<SymbolTable>(currentScope_);
    }

    void leaveScope() {
        if (currentScope_->getParent()) {
            currentScope_ = currentScope_->getParent();
        }
    }

    void define(const std::string& ns, const SymbolPtr& symbol) {
        currentScope_->define(ns, symbol);
    }

    SymbolPtr resolve(const std::string& ns, const std::string& name) const {
        return currentScope_->get(ns, name);
    }

    bool exists(const std::string& ns, const std::string& name) const {
        return currentScope_->exists(ns, name);
    }

    std::vector<SymbolPtr> listNamespace(const std::string& ns) const {
        return currentScope_->listAll(ns);
    }

    std::shared_ptr<SymbolTable> getGlobalScope() const { return globalScope_; }
    std::shared_ptr<SymbolTable> getCurrentScope() const { return currentScope_; }
};

} // namespace Symbols

#endif
