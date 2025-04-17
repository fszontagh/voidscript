#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

#include "ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class IdentifierExpressionNode : public ExpressionNode {
    std::string name_;

  public:
    explicit IdentifierExpressionNode(std::string name) : name_(std::move(name)) {}

    Symbols::Value evaluate(Interpreter & /*interpreter*/) const override {
        const auto ns = Symbols::SymbolContainer::instance()->currentScopeName() + ".variables";
        if (Symbols::SymbolContainer::instance()->exists(name_, ns)) {
            return Symbols::SymbolContainer::instance()->get(ns, name_)->getValue();
        }
        throw std::runtime_error("Variable " + name_ + " does not exist in ns: " + ns);
    }

    std::string toString() const override { return name_; }
};

}  // namespace Interpreter

#endif  // IDENTIFIER_EXPRESSION_NODE_HPP
