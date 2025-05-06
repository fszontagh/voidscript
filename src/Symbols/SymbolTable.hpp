#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <vector>
#include <string>
#include "SymbolTypes.hpp"
#include <iostream>
#include <unordered_map>

namespace Symbols {

class SymbolTable {
    // NamespaceMap symbols_; // OLD
    std::unordered_map<std::string, SymbolPtr> flat_symbols_; // NEW: Flattened map
    const std::string key_separator = "::"; // Separator for flat key

  public:
    void define(const std::string & ns, const SymbolPtr & symbol) { 
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + symbol->name();
        // // DEBUG START
        // std::cout << "[Debug][SymbolTable@" << this << "] Defining symbol '" << symbol->name() << "' with flat key '" << flat_key << "' (Symbol context: '" << symbol->context() <<"')" << std::endl;
        // // DEBUG END
        flat_symbols_[flat_key] = symbol; 
        // // DEBUG START: Verify entry
        // auto it = flat_symbols_.find(flat_key);
        // if (it != flat_symbols_.end()) {
        //     std::cout << "[Debug][SymbolTable]   VERIFIED flat entry for key '" << flat_key << "' exists after define." << std::endl;
        // } else {
        //     std::cout << "[Debug][SymbolTable]   !!! FAILED TO VERIFY flat entry for key '" << flat_key << "' immediately after define." << std::endl;
        // }
        // // DEBUG END
    }

    bool exists(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        // // DEBUG START
        // std::cout << "[Debug][SymbolTable@" << this << "] Getting symbol via flat key '" << flat_key << "' (Original name: '" << name << "', sub-ns: '" << ns << "')" << std::endl;
        // // DEBUG END
        auto it = flat_symbols_.find(flat_key);
        if (it != flat_symbols_.end()) {
             // // DEBUG START
             // std::cout << "[Debug][SymbolTable]   FOUND symbol for flat key '" << flat_key << "'" << std::endl;
             // // DEBUG END
            return it->second != nullptr;
        }
         // // DEBUG START
         // std::cout << "[Debug][SymbolTable]   Symbol NOT found for flat key '" << flat_key << "'" << std::endl;
         // // DEBUG END
        return false;
    }

    SymbolPtr get(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        // // DEBUG START
        // std::cout << "[Debug][SymbolTable@" << this << "] Getting symbol via flat key '" << flat_key << "' (Original name: '" << name << "', sub-ns: '" << ns << "')" << std::endl;
        // // DEBUG END
        auto it = flat_symbols_.find(flat_key);
        if (it != flat_symbols_.end()) {
             // // DEBUG START
             // std::cout << "[Debug][SymbolTable]   FOUND symbol for flat key '" << flat_key << "'" << std::endl;
             // // DEBUG END
            return it->second;
        }
         // // DEBUG START
         // std::cout << "[Debug][SymbolTable]   Symbol NOT found for flat key '" << flat_key << "'" << std::endl;
         // // DEBUG END
        return nullptr;
    }

    void remove(const std::string & ns, const std::string & name) {
        // ns is sub-ns like "variables"
        std::string flat_key = ns + key_separator + name;
        flat_symbols_.erase(flat_key);
    }

    // /** @brief List all namespace keys (symbol categories) in this table. */
    // // THIS METHOD IS NO LONGER VALID WITH FLAT STRUCTURE
    // std::vector<std::string> listNamespaces() const {
    //     std::vector<std::string> result;
    //     // Cannot easily list unique prefixes from flat keys
    //     return result;
    // }

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
