#ifndef INTERPRETER_VARIABLE_REFERENCE_NODE_HPP
#define INTERPRETER_VARIABLE_REFERENCE_NODE_HPP

#include <memory>
#include <stdexcept>
#include <string>

#include "ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableSymbol.hpp"

namespace Interpreter {
class VariableReferenceNode : public ExpressionNode {
    std::string variableName_;
    std::string namespace_;  // pl. "global.variables"

  public:
    VariableReferenceNode(const std::string & variableName, const std::string & ns) :
        variableName_(variableName),
        namespace_(ns) {}

    Symbols::Value evaluate(class Interpreter & /*interpreter*/) const override {
        auto symbol = Symbols::SymbolContainer::instance()->get(namespace_, variableName_);
        if (!symbol) {
            throw std::runtime_error("Variable not found: " + variableName_);
        }
        auto varSymbol = std::dynamic_pointer_cast<Symbols::VariableSymbol>(symbol);
        if (!varSymbol) {
            throw std::runtime_error("Symbol is not a variable: " + variableName_);
        }
        return varSymbol->getValue();
    }
};

};  // namespace Interpreter
#endif  // INTERPRETER_VARIABLE_REFERENCE_NODE_HPP
