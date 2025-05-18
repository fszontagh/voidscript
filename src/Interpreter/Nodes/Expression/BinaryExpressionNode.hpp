#include <iostream>
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

    Symbols::Value::ValuePtr evaluate(Interpreter & interpreter) const override {
        auto leftVal  = lhs_->evaluate(interpreter);
        auto rightVal = rhs_->evaluate(interpreter);

        // Handle NULL values in comparisons
        if (leftVal->isNULL() || rightVal->isNULL()) {
            if (op_ == "==") {
                return std::make_shared<Symbols::Value>(leftVal->isNULL() == rightVal->isNULL());
            }
            if (op_ == "!=") {
                return std::make_shared<Symbols::Value>(leftVal->isNULL() != rightVal->isNULL());
            }
            return std::make_shared<Symbols::Value>(false);  // Other comparisons with NULL are false
        }

        auto lt = leftVal->getType();
        auto rt = rightVal->getType();

        // Boolean operations
        if (lt == Symbols::Variables::Type::BOOLEAN && rt == Symbols::Variables::Type::BOOLEAN) {
            bool l = leftVal->get<bool>();
            bool r = rightVal->get<bool>();
            if (op_ == "&&") {
                return std::make_shared<Symbols::Value>(l && r);
            }
            if (op_ == "||") {
                return std::make_shared<Symbols::Value>(l || r);
            }
            if (op_ == "==") {
                return std::make_shared<Symbols::Value>(l == r);
            }
            if (op_ == "!=") {
                return std::make_shared<Symbols::Value>(l != r);
            }
            throw std::runtime_error("Unknown operator: " + op_);
        }
        // Numeric operations: int, float, double (including mixed types)
        if ((lt == Symbols::Variables::Type::INTEGER || lt == Symbols::Variables::Type::FLOAT ||
             lt == Symbols::Variables::Type::DOUBLE) &&
            (rt == Symbols::Variables::Type::INTEGER || rt == Symbols::Variables::Type::FLOAT ||
             rt == Symbols::Variables::Type::DOUBLE)) {
            // Promote to double if any operand is double
            if (lt == Symbols::Variables::Type::DOUBLE || rt == Symbols::Variables::Type::DOUBLE) {
                double l = (lt == Symbols::Variables::Type::DOUBLE) ? leftVal->get<double>() :
                           (lt == Symbols::Variables::Type::FLOAT)  ? static_cast<double>(leftVal->get<float>()) :
                                                                      static_cast<double>(leftVal->get<int>());
                double r = (rt == Symbols::Variables::Type::DOUBLE) ? rightVal->get<double>() :
                           (rt == Symbols::Variables::Type::FLOAT)  ? static_cast<double>(rightVal->get<float>()) :
                                                                      static_cast<double>(rightVal->get<int>());
                if (op_ == "+") {
                    return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return std::make_shared<Symbols::Value>(l / r);
                }
                if (op_ == "==") {
                    return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return std::make_shared<Symbols::Value>(l >= r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }
            // Promote to float if any operand is float
            else if (lt == Symbols::Variables::Type::FLOAT || rt == Symbols::Variables::Type::FLOAT) {
                float l = (lt == Symbols::Variables::Type::FLOAT) ? leftVal->get<float>() :
                                                                    static_cast<float>(leftVal->get<int>());
                float r = (rt == Symbols::Variables::Type::FLOAT) ? rightVal->get<float>() :
                                                                    static_cast<float>(rightVal->get<int>());
                if (op_ == "+") {
                    return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return std::make_shared<Symbols::Value>(l / r);
                }
                if (op_ == "==") {
                    return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return std::make_shared<Symbols::Value>(l >= r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }
            // Both operands are integers
            else {
                int l = leftVal->get<int>();
                int r = rightVal->get<int>();
                if (op_ == "+") {
                    return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return std::make_shared<Symbols::Value>(l / r);
                }
                if (op_ == "%") {
                    return std::make_shared<Symbols::Value>(l % r);
                }
                if (op_ == "==") {
                    return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return std::make_shared<Symbols::Value>(l >= r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }
        }
        // Integer operations
        if (lt == Symbols::Variables::Type::INTEGER && rt == Symbols::Variables::Type::INTEGER) {
            if (leftVal->getType() == Symbols::Variables::Type::INTEGER &&
                rightVal->getType() == Symbols::Variables::Type::INTEGER) {
                int l = leftVal->get<int>();
                int r = rightVal->get<int>();

                if (op_ == "+") {
                    return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return std::make_shared<Symbols::Value>(l / r);  // TODO: 0 div
                }
                if (op_ == "%") {
                    return std::make_shared<Symbols::Value>(l % r);
                }
                if (op_ == "==") {
                    return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return std::make_shared<Symbols::Value>(l >= r);
                }

                throw std::runtime_error("Unknown operator: " + op_);
            }
        }

        // String operations
        if (leftVal->getType() == Symbols::Variables::Type::STRING &&
            rightVal->getType() == Symbols::Variables::Type::STRING) {
            auto l = leftVal->get<std::string>();
            auto r = rightVal->get<std::string>();

            if (op_ == "+") {
                return std::make_shared<Symbols::Value>(l + r);
            }
            if (op_ == "==") {
                return std::make_shared<Symbols::Value>(l == r);
            }
            if (op_ == "!=") {
                return std::make_shared<Symbols::Value>(l != r);
            }
            throw std::runtime_error("Unknown operator: " + op_);
        }

        throw std::runtime_error("Unsupported types in binary expression: " + Symbols::Variables::TypeToString(lt) +
                                 " and " + Symbols::Variables::TypeToString(rt) + " " + toString());

        return std::make_shared<Symbols::Value>();
    };

    std::string toString() const override { return "(" + lhs_->toString() + " " + op_ + " " + rhs_->toString() + ")"; }
};  // class
}  // namespace Interpreter
