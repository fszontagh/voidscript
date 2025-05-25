// FunctionSymbol.hpp
#ifndef FUNCTION_SYMBOL_HPP
#define FUNCTION_SYMBOL_HPP

#include <vector>

#include "BaseSymbol.hpp"
#include "Lexer/Token.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/FunctionParameterInfo.hpp"

namespace Symbols {

class FunctionSymbol : public Symbol {
    // store the variables name and type
    std::vector<FunctionParameterInfo> parameters_;
    Symbols::Variables::Type          returnType_;
    std::string                       plainBody_;
    std::vector<Lexer::Tokens::Token> tokens_;

  public:
    FunctionSymbol(const std::string & name, const std::string & context, const std::vector<FunctionParameterInfo> & parameters,
                   const std::string &      plainbody  = "",
                   Symbols::Variables::Type returnType = Symbols::Variables::Type::NULL_TYPE) :
        Symbol(name, "", context, Symbols::Kind::Function),
        parameters_(parameters),
        returnType_(returnType),
        plainBody_(plainbody) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Function; }

    Symbols::Variables::Type returnType() const { return returnType_; }

    const std::vector<FunctionParameterInfo> & parameters() const { return parameters_; }

    const std::string & plainBody() const { return plainBody_; }

    // Dump function symbol: name, context, and declared return type
    std::string dump() const override {
        std::string r = "\t\t  " + kindToString(this->kind_) + " name: '" + name_ + "' \n\t\t\tContext: " + context_;
        r += "\n\t\t\tArgs (" + std::to_string(this->parameters_.size()) + "): \n";
        for (const auto & p : parameters_) {
            r += "\t\t\t - " + p.name;
            r += Symbols::Variables::TypeToString(p.type) + "\n";
        }
        r += " \n\t\t\tReturnType: " + Variables::TypeToString(returnType_);
        return r;
    }
};

}  // namespace Symbols

#endif
