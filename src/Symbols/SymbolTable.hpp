#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <vector>
#include <string>
#include "SymbolTypes.hpp"

namespace Symbols {

class SymbolTable {
    NamespaceMap symbols_;

  public:
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
        return nullptr;
    }

    void remove(const std::string & ns, const std::string & name) {
        auto itNs = symbols_.find(ns);
        if (itNs != symbols_.end()) {
            itNs->second.erase(name);
        }
    }

    std::vector<std::string> listNSs() {
        std::vector<std::string> result;
        for (const auto & [ns, _] : symbols_) {
            result.push_back(ns);
        }
        return result;
    }

    std::vector<SymbolPtr> listAll(const std::string & prefix = "") const {
        std::vector<SymbolPtr> result;
        for (const auto & [ns, map] : symbols_) {
            if (prefix.empty() || ns.substr(0,prefix.length()) == prefix) {
                for (const auto & [_, sym] : map) {
                    result.push_back(sym);
                }
            }
        }
        return result;
    }

    void clear(const std::string & ns) { symbols_.erase(ns); }

    void clearAll() { symbols_.clear(); }
};

}  // namespace Symbols

#endif
