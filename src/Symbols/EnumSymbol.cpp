#include "EnumSymbol.hpp"
#include <stdexcept> // For std::runtime_error
#include <sstream>   // For std::ostringstream in dump

namespace Symbols {

// Constructor
EnumSymbol::EnumSymbol(
    const std::string& enumName,
    const std::vector<std::pair<std::string, std::optional<int>>>& enumeratorDefs,
    const std::string& context
) : Symbol(enumName, ValuePtr(nullptr), context, Symbols::Kind::ENUM) { // Pass Symbols::Kind::ENUM to base
    int nextValue = 0;
    for (const auto& def : enumeratorDefs) {
        const std::string& name = def.first;
        std::optional<int> valOpt = def.second;

        if (enumerators_.count(name)) {
            throw std::runtime_error("Duplicate enumerator name: " + name + " in enum " + enumName);
        }

        int currentValue;
        if (valOpt.has_value()) {
            currentValue = valOpt.value();
        } else {
            currentValue = nextValue;
        }
        
        enumerators_[name] = currentValue;
        nextValue = currentValue + 1;
    }
}

// Override virtual methods from BaseSymbol
Symbols::Kind EnumSymbol::kind() const {
    return Symbols::Kind::ENUM; // Or use this->kind_ directly if preferred and accessible
}

std::string EnumSymbol::dump() const {
    std::ostringstream oss;
    oss << "\t\t  " << kindToString(this->kind_) << " name: '" << this->name_ << "' \n";
    oss << "\t\t\tContext: " << this->context_ << "\n";
    oss << "\t\t\tEnumerators: {\n";
    for (const auto& pair : enumerators_) {
        oss << "\t\t\t  " << pair.first << ": " << pair.second << "\n";
    }
    oss << "\t\t\t}";
    return oss.str();
}

// EnumSymbol specific methods
std::optional<int> EnumSymbol::GetValue(const std::string& enumeratorName) const {
    auto it = enumerators_.find(enumeratorName);
    if (it != enumerators_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool EnumSymbol::HasEnumerator(const std::string& enumeratorName) const {
    return enumerators_.count(enumeratorName) > 0;
}

} // namespace Symbols
