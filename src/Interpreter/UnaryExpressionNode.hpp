#ifndef UNARY_EXPRESSION_NODE_HPP
#define UNARY_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "ExpressionNode.hpp"

namespace Interpreter {

class UnaryExpressionNode : public ExpressionNode {
    std::string                     op_;
    std::unique_ptr<ExpressionNode> operand_;

  public:
    UnaryExpressionNode(std::string op, std::unique_ptr<ExpressionNode> operand) :
        op_(std::move(op)),
        operand_(std::move(operand)) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        auto value = operand_->evaluate(interpreter);
        auto type  = value.getType();

        if (type == Symbols::Variables::Type::INTEGER) {
            int v = value.get<int>();
            if (op_ == "-") {
                return Symbols::Value(-v);
            }
            if (op_ == "+") {
                return Symbols::Value(+v);
            }
        } else if (type == Symbols::Variables::Type::DOUBLE) {
            double v = value.get<double>();
            if (op_ == "-") {
                return Symbols::Value(-v);
            }
            if (op_ == "+") {
                return Symbols::Value(+v);
            }
        } else if (type == Symbols::Variables::Type::FLOAT) {
            float v = value.get<float>();
            if (op_ == "-") {
                return Symbols::Value(-v);
            }
            if (op_ == "+") {
                return Symbols::Value(+v);
            }
        } else if (type == Symbols::Variables::Type::BOOLEAN) {
            bool v = value.get<bool>();
            if (op_ == "!") {
                return Symbols::Value(!v);
            }
        }

        throw std::runtime_error("Unsupported unary operator '" + op_ +
                                 "' for type: " + Symbols::Variables::TypeToString(type));
    }

    std::string toString() const override { return "(" + op_ + operand_->toString() + ")"; }
};

}  // namespace Interpreter

#endif  // UNARY_EXPRESSION_NODE_HPP
