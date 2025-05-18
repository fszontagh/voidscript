#ifndef LITERAL_EXPRESSION_NODE_HPP
#define LITERAL_EXPRESSION_NODE_HPP

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class LiteralExpressionNode : public ExpressionNode {
    Symbols::Value::ValuePtr value_;

  public:
    explicit LiteralExpressionNode(Symbols::Value::ValuePtr value) : value_(std::move(value)) {}

    Symbols::Value::ValuePtr evaluate(class Interpreter & /*interpreter*/) const override { return value_; }

    Symbols::Value::ValuePtr value() const { return value_; }

    // to string
    std::string toString() const override { return Symbols::Value::to_string(value_); }
};

}  // namespace Interpreter

#endif  // LITERAL_EXPRESSION_NODE_HPP
