#ifndef LITERAL_EXPRESSION_NODE_HPP
#define LITERAL_EXPRESSION_NODE_HPP

#include "ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class LiteralExpressionNode : public ExpressionNode {
    Symbols::Value value_;

  public:
    explicit LiteralExpressionNode(Symbols::Value value) : value_(std::move(value)) {}

    Symbols::Value evaluate(class Interpreter & /*interpreter*/) const override { return value_; }

    const Symbols::Value & value() const { return value_; }

    // to string
    std::string toString() const override { return Symbols::Value::to_string(value_); }
};

}  // namespace Interpreter

#endif  // LITERAL_EXPRESSION_NODE_HPP
