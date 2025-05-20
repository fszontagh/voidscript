#ifndef LITERAL_EXPRESSION_NODE_HPP
#define LITERAL_EXPRESSION_NODE_HPP

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class LiteralExpressionNode : public ExpressionNode {
    Symbols::ValuePtr value_;

  public:
    explicit LiteralExpressionNode(const Symbols::ValuePtr & value) : value_(value) {}

    Symbols::ValuePtr evaluate(class Interpreter & /*interpreter*/, std::string /*filename*/, int /*line*/,
                               size_t /*col*/) const override {
        return value_;
    }

    Symbols::ValuePtr & value() { return value_; }

    // to string
    std::string toString() const override { return value_.toString(); }
};

}  // namespace Interpreter

#endif  // LITERAL_EXPRESSION_NODE_HPP
