// SymbolTable.hpp
#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>
#include <vector>

#include "SymbolTypes.hpp"

namespace Symbols {

class SymbolTable {
    NamespaceMap                 symbols_;
    std::shared_ptr<SymbolTable> parent_ = nullptr;

  public:
    SymbolTable(std::shared_ptr<SymbolTable> parent = nullptr) : parent_(std::move(parent)) {}

    void define(const std::string & ns, const SymbolPtr & symbol) { symbols_[ns][symbol->name()] = symbol; }

    bool exists(const std::string & ns, const std::string & name) const { return get(ns, name) != nullptr; }

    SymbolPtr get(const std::string & ns, const std::string & name) const {
        auto itNs = symbols_.find(ns);
        if (itNs != symbols_.end()) {
            const auto & map = itNs->second;
            auto         it  = map.find(name);
            if (it != map.end()) {
                return it->second;
            }
        }
        // Rekurzívan keresünk a szülő scope-ban
        if (parent_) {
            return parent_->get(ns, name);
        }
        return nullptr;
    }

    void remove(const std::string & ns, const std::string & name) {
        auto itNs = symbols_.find(ns);
        if (itNs != symbols_.end()) {
            itNs->second.erase(name);
        }
    }

    std::vector<SymbolPtr> listAll(const std::string & ns) const {
        std::vector<SymbolPtr> result;
        auto                   it = symbols_.find(ns);
        if (it != symbols_.end()) {
            for (const auto & [_, sym] : it->second) {
                result.push_back(sym);
            }
        }
        return result;
    }

    void clear(const std::string & ns) { symbols_.erase(ns); }

    void clearAll() { symbols_.clear(); }

    std::shared_ptr<SymbolTable> getParent() const { return parent_; }
};

}  // namespace Symbols

#endif
