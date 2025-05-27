// SymbolKind.hpp
#ifndef SYMBOL_KIND_HPP
#define SYMBOL_KIND_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Symbols {

enum class Kind : std::uint8_t {
    Variable,
    Constant,
    Function,
    Method,   // Method belongs to a class
    Class,    // Class definition
    ENUM      // Enum definition
    // Later: Module, etc..
};

static std::string kindToString(Symbols::Kind kind) {
    std::unordered_map<Symbols::Kind, std::string> KindToString = {
        { Symbols::Kind::Variable, "Variable" },
        { Symbols::Kind::Constant, "Constant" },
        { Symbols::Kind::Function, "Function" },
        { Symbols::Kind::Method,   "Method"   },
        { Symbols::Kind::Class,    "Class"    },
        { Symbols::Kind::ENUM,     "Enum"     },
    };

    auto it = KindToString.find(kind);
    if (it != KindToString.end()) {
        return it->second;
    }
    return "Unknown kind: " + std::to_string(static_cast<int>(kind));
}
};  // namespace Symbols

#endif
