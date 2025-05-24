// SymbolTypes.hpp
#ifndef SYMBOL_TYPES_HPP
#define SYMBOL_TYPES_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "BaseSymbol.hpp"

namespace Symbols {


using SymbolPtr = std::shared_ptr<Symbol>;

// Namespace -> name -> symbol
using SymbolMap    = std::unordered_map<std::string, SymbolPtr>;
using NamespaceMap = std::unordered_map<std::string, SymbolMap>;

}  // namespace Symbols

#endif
