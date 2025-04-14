// ConstantSymbol.hpp
#ifndef CONSTANT_SYMBOL_HPP
#define CONSTANT_SYMBOL_HPP

#include <stdexcept>

#include "BaseSymbol.hpp"
#include "VariableTypes.hpp"

namespace Symbols {

class ConstantSymbol : public Symbol {
  protected:
    Symbols::Variables::Type _vartype;
  public:
    ConstantSymbol(const std::string & name, const Symbols::Value & value, const std::string & context) :
        Symbol(name, value, context, Symbols::Kind::Constant) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Constant; }

    void setValue(const Symbols::Value & /*value*/) override {
        throw std::logic_error("Cannot modify a constant symbol");
    }
};

}  // namespace Symbols

#endif
