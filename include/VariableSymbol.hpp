// VariableSymbol.hpp
#ifndef VARIABLE_SYMBOL_HPP
#define VARIABLE_SYMBOL_HPP

#include "BaseSymbol.hpp"

namespace Symbols {

class VariableSymbol : public Symbol {
  public:
    VariableSymbol(const std::string & name, const Symbols::Value & value, const std::string & context,
                   Symbols::Kind type) :
        Symbol(name, value, context, type) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Variable; }
};

}  // namespace Symbols

#endif
