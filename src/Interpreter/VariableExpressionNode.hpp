#ifndef VARIABLEEXPRESSIONNODE_HPP
#define VARIABLEEXPRESSIONNODE_HPP

#include <string>
#include <utility>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {
class VariableExpressionNode : public ExpressionNode {
    std::string variableName_;
    std::string ns;

  public:
    VariableExpressionNode(std::string varName, std::string ns) :
        variableName_(std::move(varName)),
        ns(std::move(ns)) {}

    Symbols::Value evaluate(Interpreter & /*interpreter*/) const override {
        if (!Symbols::SymbolContainer::instance()->exists(variableName_)) {
            throw std::runtime_error("Variable not found: " + variableName_);
        }
        auto symbol = Symbols::SymbolContainer::instance()->get(ns, variableName_);
        return symbol->getValue();
    }

    std::string toString() const override { return "$" + variableName_; }
};

};  // namespace Interpreter

#endif  // VARIABLEEXPRESSIONNODE_HPP
