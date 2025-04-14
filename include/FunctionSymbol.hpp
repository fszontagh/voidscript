// FunctionSymbol.hpp
#ifndef FUNCTION_SYMBOL_HPP
#define FUNCTION_SYMBOL_HPP

#include <vector>

#include "BaseSymbol.hpp"

namespace Symbols {
using ValueContainer = std::vector<Symbols::Value>;

class FunctionSymbol : public Symbol {
    std::vector<Symbols::Value> parameters_;
    Symbols::Value              returnType_;
    std::string                 plainBody_;

  public:
    FunctionSymbol(const std::string & name, const std::string & context, const ValueContainer & parameters,
                   const std::string & plainbody = "") :
        Symbol(name, {}, context, Symbols::Kind::Function),
        parameters_(parameters),
        plainBody_(plainbody) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Function; }

    const ValueContainer & parameters() const { return parameters_; }
};

}  // namespace Symbols

#endif
