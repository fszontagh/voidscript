#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <vector>
#include <string>
#include "SymbolTypes.hpp"
#include <unordered_map>

namespace Symbols {

class SymbolTable {
    // NamespaceMap symbols_; // OLD
    std::unordered_map<std::string, SymbolPtr> flat_symbols_; // NEW: Flattened map
    const std::string key_separator = "::"; // Separator for flat key

  public:
  SymbolTable(const std::string &separator) : key_separator(separator) {}
    void define(const std::string & ns, const SymbolPtr & symbol) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + symbol->name();
        flat_symbols_[flat_key] = symbol;
    }

    bool exists(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        auto it = flat_symbols_.find(flat_key);
        if (it != flat_symbols_.end()) {
            return it->second != nullptr;
        }
        return false;
    }

    SymbolPtr get(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        auto it = flat_symbols_.find(flat_key);
        if (it != flat_symbols_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void remove(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        flat_symbols_.erase(flat_key);
    }


    std::vector<SymbolPtr> listAll(const std::string & prefix_ns = "") const {
        std::vector<SymbolPtr> result;
        std::string prefix_key = prefix_ns.empty() ? "" : (prefix_ns + key_separator);
        for (const auto & [key, sym] : flat_symbols_) {
            if (prefix_key.empty() || key.rfind(prefix_key, 0) == 0) { // Check if key starts with prefix_key
                 result.push_back(sym);
            }
        }
        return result;
    }

    void clear(const std::string & ns) {
        // ns is sub-ns like "variables"
        std::string prefix_key = ns + key_separator;
        for (auto it = flat_symbols_.begin(); it != flat_symbols_.end(); /* no increment */) {
            if (it->first.rfind(prefix_key, 0) == 0) { // Check if key starts with prefix_key
                it = flat_symbols_.erase(it); // Erase and advance iterator
            } else {
                ++it; // Advance iterator
            }
        }
     }

    void clearAll() { flat_symbols_.clear(); }
};

}  // namespace Symbols

#endif
