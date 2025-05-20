// VariableSymbol.hpp
#ifndef VARIABLE_SYMBOL_HPP
#define VARIABLE_SYMBOL_HPP

#include "BaseSymbol.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class VariableSymbol : public Symbol {
  protected:
    Symbols::Variables::Type type_;
  public:
    VariableSymbol(const std::string & name, const ValuePtr & value, const std::string & context, Variables::Type type) :
        Symbols::Symbol(name, value, context, Symbols::Kind::Variable),
        type_(type) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Variable; }

    Variables::Type type() const { return type_; }

    std::string toString() const {
        std::string r = "VariableSymbol: " + name_ + " Type: " + Symbols::Variables::TypeToString(type_);
        r += "Value: " + value_.toString();
        return r;
    }
};

}  // namespace Symbols

#endif
