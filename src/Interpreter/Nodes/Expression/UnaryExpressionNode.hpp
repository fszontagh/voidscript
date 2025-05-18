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
        auto value = operand_->evaluate(interpreter);
        auto type  = value->getType();

        if (type == Symbols::Variables::Type::INTEGER) {
            int v = value->get<int>();
            if (op_ == "-") {
                return std::make_shared<Symbols::Value>(-v);
            }
            if (op_ == "+") {
                return std::make_shared<Symbols::Value>(+v);
            }
            if (op_ == "++") {
                return std::make_shared<Symbols::Value>(v + 1);
            }
            if (op_ == "--") {
                return std::make_shared<Symbols::Value>(v - 1);
            }
        } else if (type == Symbols::Variables::Type::DOUBLE) {
            double v = value->get<double>();
            if (op_ == "-") {
                return std::make_shared<Symbols::Value>(-v);
            }
            if (op_ == "+") {
                return std::make_shared<Symbols::Value>(+v);
            }
            if (op_ == "++") {
                return std::make_shared<Symbols::Value>(v + 1);
            }
            if (op_ == "--") {
                return std::make_shared<Symbols::Value>(v - 1);
            }
        } else if (type == Symbols::Variables::Type::FLOAT) {
            float v = value->get<float>();
            if (op_ == "-") {
                return std::make_shared<Symbols::Value>(-v);
            }
            if (op_ == "+") {
                return std::make_shared<Symbols::Value>(+v);
            }
            if (op_ == "++") {
                return std::make_shared<Symbols::Value>(v + 1);
            }
            if (op_ == "--") {
                return std::make_shared<Symbols::Value>(v - 1);
            }
        } else if (type == Symbols::Variables::Type::BOOLEAN) {
            bool v = value->get<bool>();
            if (op_ == "!") {
                return std::make_shared<Symbols::Value>(!v);
            }
        } else if (type == Symbols::Variables::Type::STRING) {
            std::string s = value->get<std::string>();
            if (op_ == "-") {
                return std::make_shared<Symbols::Value>(s);
            }
            if (op_ == "+") {
                return std::make_shared<Symbols::Value>(s);
            }
        }

        throw std::runtime_error("Unsupported unary operator '" + op_ +
                                 "' for type: " + Symbols::Variables::TypeToString(type));
    }

    std::string toString() const override { return "(" + op_ + operand_->toString() + ")"; }
};

}  // namespace Interpreter

#endif  // UNARY_EXPRESSION_NODE_HPP
