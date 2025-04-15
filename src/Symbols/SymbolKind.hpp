// SymbolKind.hpp
#ifndef SYMBOL_KIND_HPP
#define SYMBOL_KIND_HPP

#include <cstdint>

namespace Symbols {

enum class Kind : std::uint8_t {
    Variable,
    Constant,
    Function
    // Later: Module, Class, etc..
};

}; // namespace Symbols

#endif
