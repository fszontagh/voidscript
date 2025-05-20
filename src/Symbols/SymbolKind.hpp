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
    Function
    // Later: Module, Class, etc..
};

static std::string kindToString(Symbols::Kind kind) {
    switch (kind) {
        case Symbols::Kind::Variable: return "Variable";
        case Symbols::Kind::Constant: return "Constant";
        case Symbols::Kind::Function: return "Function";
        default:
            return "Unknown kind: " + std::to_string(static_cast<int>(kind));
    }
}
>>>>>>> REPLable REPLACE
```

2. Let's simplify the `SymbolContainer.cpp` file:

src/SymbolContainer.cpp
```cpp
<<<<<<< SEARCH
namespace Symbols {
    std::string SymbolContainer::initial_scope_name_for_singleton_;
    bool SymbolContainer::is_initialized_for_singleton_ = false;
} // namespace Symbols
};  // namespace Symbols

#endif
