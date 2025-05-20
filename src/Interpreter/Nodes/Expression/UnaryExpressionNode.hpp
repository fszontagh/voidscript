#ifndef UNARY_EXPRESSION_NODE_HPP
#define UNARY_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"

namespace Interpreter {

class UnaryExpressionNode : public ExpressionNode {
    std::string                     op_;
    std::unique_ptr<ExpressionNode> operand_;

  public:
    UnaryExpressionNode(std::string op, std::unique_ptr<ExpressionNode> operand) :
        op_(std::move(op)),
        operand_(std::move(operand)) {}

    Symbols::ValuePtr evaluate(Interpreter & interpreter) const override {
        const auto value = operand_->evaluate(interpreter);

        if (value == Symbols::Variables::Type::INTEGER) {
            int v = value;
            if (op_ == "-") {
                return -v;
                //return Symbols::ValuePtr(-v);
            }
            if (op_ == "+") {
                return +v;
                //return Symbols::ValuePtr(+v);
            }
            if (op_ == "++") {
                return v + 1;
                //return Symbols::ValuePtr(v + 1);
            }
            if (op_ == "--") {
                return v - 1;
                //return Symbols::ValuePtr(v - 1);
            }
        } else if (value == Symbols::Variables::Type::DOUBLE) {
            double v = value;
            if (op_ == "-") {
                return -v;
                //return Symbols::ValuePtr(-v);
            }
            if (op_ == "+") {
                return +v;
                //return Symbols::ValuePtr(+v);
            }
            if (op_ == "++") {
                return v + 1;
                //return Symbols::ValuePtr(v + 1);
            }
            if (op_ == "--") {
                return v - 1;
                //return Symbols::ValuePtr(v - 1);
            }
        } else if (value == Symbols::Variables::Type::FLOAT) {
            float v = value;  //->get<float>();
            if (op_ == "-") {
                return -v;
                //return Symbols::ValuePtr(-v);
            }
            if (op_ == "+") {
                return +v;
                //return Symbols::ValuePtr(+v);
            }
            if (op_ == "++") {
                return v + 1;
                //return Symbols::ValuePtr(v + 1);
            }
            if (op_ == "--") {
                return v - 1;
                //return Symbols::ValuePtr(v - 1);
            }
        } else if (value == Symbols::Variables::Type::BOOLEAN) {
            bool v = value;  //->get<bool>();
            if (op_ == "!") {
                return !v;
                //return Symbols::ValuePtr(!v);
            }
        } else if (value == Symbols::Variables::Type::STRING) {
            const std::string s = value;  //->get<std::string>();
            if (op_ == "-") {
                return s;
                //return Symbols::ValuePtr(s);
            }
            if (op_ == "+") {
                return s;
                //return Symbols::ValuePtr(s);
            }
        }

        throw std::runtime_error("Unsupported unary operator '" + op_ +
                                 "' for type: " + Symbols::Variables::TypeToString(value));
    }

    std::string toString() const override { return "(" + op_ + operand_->toString() + ")"; }
};

}  // namespace Interpreter

#endif  // UNARY_EXPRESSION_NODE_HPP
