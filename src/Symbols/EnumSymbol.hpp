#ifndef ENUM_SYMBOL_HPP
#define ENUM_SYMBOL_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept> // For std::runtime_error
#include <utility> // For std::pair

#include "BaseSymbol.hpp"
#include "SymbolKind.hpp"
#include "Value.hpp" // Required by BaseSymbol and for the constructor

namespace Symbols {

class EnumSymbol : public Symbol {
private:
    std::map<std::string, int> enumerators_;

public:
    // Constructor
    EnumSymbol(
        const std::string& enumName,
        const std::vector<std::pair<std::string, std::optional<int>>>& enumeratorDefs,
        const std::string& context // context (namespace) for the BaseSymbol
    );

    // Override virtual methods from BaseSymbol
    Symbols::Kind kind() const override;
    std::string dump() const override; // Changed from ToString to match BaseSymbol's interface

    // EnumSymbol specific methods
    std::optional<int> GetValue(const std::string& enumeratorName) const;
    bool HasEnumerator(const std::string& enumeratorName) const;

    const std::map<std::string, int>& getEnumerators() const { return enumerators_; }
};

} // namespace Symbols

#endif // ENUM_SYMBOL_HPP
