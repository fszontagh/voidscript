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

    Symbols::ValuePtr evaluate(Interpreter & /*interpreter*/, std::string /*filename*/ = "", int /*line*/ = 0, size_t /*column*/ = 0) const override {
        // Use getVariable which already handles scope traversal from innermost to outermost
        auto* sc = Symbols::SymbolContainer::instance();
        auto symbol = sc->getVariable(variableName_);
        
        // DEBUG: Print lookup information
        std::cerr << "VariableExpressionNode: looking up variable '" << variableName_
                  << "' in current scope '" << sc->currentScopeName() << "'" << std::endl;
        
        if (!symbol) {
            std::cerr << "VariableExpressionNode: variable '" << variableName_ << "' not found!" << std::endl;
            throw std::runtime_error("Undefined variable name: " + variableName_);
        }
        return symbol->getValue();
    }

    std::string toString() const override { return "$" + variableName_; }
};

};  // namespace Interpreter

#endif  // VARIABLEEXPRESSIONNODE_HPP
