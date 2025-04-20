#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"

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
        using namespace Symbols::Variables;
        auto lt = leftVal.getType();
        auto rt = rightVal.getType();
        // Boolean operations
        if (lt == Type::BOOLEAN && rt == Type::BOOLEAN) {
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
        // Numeric operations: int, float, double (including mixed types)
        if ((lt == Type::INTEGER || lt == Type::FLOAT || lt == Type::DOUBLE) &&
            (rt == Type::INTEGER || rt == Type::FLOAT || rt == Type::DOUBLE)) {
            // Promote to double if any operand is double
            if (lt == Type::DOUBLE || rt == Type::DOUBLE) {
                double l = (lt == Type::DOUBLE) ? leftVal.get<double>() :
                           (lt == Type::FLOAT)  ? static_cast<double>(leftVal.get<float>()) :
                                                  static_cast<double>(leftVal.get<int>());
                double r = (rt == Type::DOUBLE) ? rightVal.get<double>() :
                           (rt == Type::FLOAT)  ? static_cast<double>(rightVal.get<float>()) :
                                                  static_cast<double>(rightVal.get<int>());
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
                    return Symbols::Value(l / r);
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
            }
            // Promote to float if any operand is float (and none is double)
            else if (lt == Type::FLOAT || rt == Type::FLOAT) {
                float l = (lt == Type::FLOAT) ? leftVal.get<float>() : static_cast<float>(leftVal.get<int>());
                float r = (rt == Type::FLOAT) ? rightVal.get<float>() : static_cast<float>(rightVal.get<int>());
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
                    return Symbols::Value(l / r);
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
            }
            // Integer case will be handled below
        }
        // Integer operations
        if (lt == Type::INTEGER && rt == Type::INTEGER) {
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
                if (op_ == "==") {
                    return Symbols::Value(l == r);
                }
                if (op_ == "!=") {
                    return Symbols::Value(l != r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }

            throw std::runtime_error("Unsupported types in binary expression: " + TypeToString(lt) + " and " +
                                     TypeToString(rt) + " " + toString());
        }
        return Symbols::Value();
    };

    std::string toString() const override { return "(" + lhs_->toString() + " " + op_ + " " + rhs_->toString() + ")"; }
};  // class
}  // namespace Interpreter
