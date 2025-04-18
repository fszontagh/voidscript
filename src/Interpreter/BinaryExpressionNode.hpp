#include <memory>
#include <string>

#include "ExpressionNode.hpp"

namespace Interpreter {
class BinaryExpressionNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode> lhs_;
    std::unique_ptr<ExpressionNode> rhs_;
    std::string                     op_;

  public:
    BinaryExpressionNode(std::unique_ptr<ExpressionNode> lhs, std::string op, std::unique_ptr<ExpressionNode> rhs) :
        lhs_(std::move(lhs)),
        rhs_(std::move(rhs)),
        op_(std::move(op)) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        auto leftVal  = lhs_->evaluate(interpreter);
        auto rightVal = rhs_->evaluate(interpreter);
        if (leftVal.getType() != rightVal.getType()) {
            throw std::runtime_error(
                "Unsupported types in binary expression: " + Symbols::Variables::TypeToString(leftVal.getType()) +
                " and " + Symbols::Variables::TypeToString(rightVal.getType()) + " " + toString());
        }
        if (leftVal.getType() == Symbols::Variables::Type::BOOLEAN &&
            rightVal.getType() == Symbols::Variables::Type::BOOLEAN) {
            bool l = leftVal.get<bool>();
            bool r = rightVal.get<bool>();

            if (op_ == "&&") {
                return Symbols::Value(l && r);
            }
            if (op_ == "||") {
                return Symbols::Value(l || r);
            }
            if (op_ == "==") {
                return Symbols::Value(l == r);
            }
            if (op_ == "!=") {
                return Symbols::Value(l != r);
            }

            throw std::runtime_error("Unknown operator: " + op_);
        }

        if (leftVal.getType() == Symbols::Variables::Type::INTEGER &&
            rightVal.getType() == Symbols::Variables::Type::INTEGER) {
            int l = leftVal.get<int>();
            int r = rightVal.get<int>();

            if (op_ == "+") {
                return Symbols::Value(l + r);
            }
            if (op_ == "-") {
                return Symbols::Value(l - r);
            }
            if (op_ == "*") {
                return Symbols::Value(l * r);
            }
            if (op_ == "/") {
                return Symbols::Value(l / r);  // TODO: 0 div
            }
            if (op_ == "%") {
                return Symbols::Value(l % r);
            }
            if (op_ == "==") {
                return Symbols::Value(l == r);
            }
            if (op_ == "!=") {
                return Symbols::Value(l != r);
            }
            if (op_ == "<") {
                return Symbols::Value(l < r);
            }
            if (op_ == ">") {
                return Symbols::Value(l > r);
            }
            if (op_ == "<=") {
                return Symbols::Value(l <= r);
            }
            if (op_ == ">=") {
                return Symbols::Value(l >= r);
            }

            throw std::runtime_error("Unknown operator: " + op_);
        }

        if (leftVal.getType() == Symbols::Variables::Type::STRING &&
            rightVal.getType() == Symbols::Variables::Type::STRING) {
            auto l = leftVal.get<std::string>();
            auto r = rightVal.get<std::string>();

            if (op_ == "+") {
                return Symbols::Value(l + r);
            }

            throw std::runtime_error("Unknown operator: " + op_);
        }

        throw std::runtime_error(
            "Unsupported types in binary expression: " + Symbols::Variables::TypeToString(leftVal.getType()) + " and " +
            Symbols::Variables::TypeToString(rightVal.getType()) + " " + toString());
    }

    std::string toString() const override { return "(" + lhs_->toString() + " " + op_ + " " + rhs_->toString() + ")"; }
};

};  // namespace Interpreter
