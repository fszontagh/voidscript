// MethodSymbol.hpp
#ifndef METHOD_SYMBOL_HPP
#define METHOD_SYMBOL_HPP

#include <vector>
#include <string>

#include "FunctionSymbol.hpp"
#include "Lexer/Token.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class MethodSymbol : public FunctionSymbol {
private:
    std::string className_; // The class this method belongs to

public:
    MethodSymbol(const std::string & name, const std::string & context, const std::string & className,
                 const std::vector<FunctionParameterInfo> & parameters,
                 const std::string &      plainbody  = "",
                 Symbols::Variables::Type returnType = Symbols::Variables::Type::NULL_TYPE) :
        FunctionSymbol(name, context, parameters, plainbody, returnType),
        className_(className) {
        // Override the kind to be Method instead of Function
        this->kind_ = Symbols::Kind::Method;
    }

    Symbols::Kind kind() const override { return Symbols::Kind::Method; }
    
    const std::string & className() const { return className_; }

    // Dump method symbol: name, class name, context, and declared return type
    std::string dump() const override {
        std::string r = "\t\t  " + kindToString(this->kind()) + " name: '" + this->name() + 
            "' \n\t\t\tClass: " + className_ + " \n\t\t\tContext: " + this->context();
        r += "\n\t\t\tArgs (" + std::to_string(this->parameters().size()) + "): \n";
        for (const auto & p : this->parameters()) {
            r += "\t\t\t - " + p.name;
            r += Symbols::Variables::TypeToString(p.type) + "\n";
        }
        r += " \n\t\t\tReturnType: " + Variables::TypeToString(this->returnType());
        return r;
    }
};

}  // namespace Symbols

#endif
