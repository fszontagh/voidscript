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

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string filename, int line,
                               size_t column) const override {
        auto leftVal  = lhs_->evaluate(interpreter, filename, line, column);
        auto rightVal = rhs_->evaluate(interpreter, filename, line, column);

        // Handle NULL values in comparisons
        if (leftVal->isNULL() || rightVal->isNULL()) {
            if (op_ == "==") {
                return leftVal->isNULL() == rightVal->isNULL();
            }
            if (op_ == "!=") {
                return leftVal->isNULL() != rightVal->isNULL();
            }
            return false;
        }

        // Boolean operations
        if (leftVal == Symbols::Variables::Type::BOOLEAN && rightVal == Symbols::Variables::Type::BOOLEAN) {
            bool l = leftVal;
            bool r = rightVal;
            if (op_ == "&&") {
                return l && r;
                //return std::make_shared<Symbols::Value>(l && r);
            }
            if (op_ == "||") {
                return l || r;
                //return std::make_shared<Symbols::Value>(l || r);
            }
            if (op_ == "==") {
                return l == r;
            }
            if (op_ == "!=") {
                return l != r;
                //return std::make_shared<Symbols::Value>(l != r);
            }
            throw std::runtime_error("Unknown operator: " + op_);
        }
        // Numeric operations: int, float, double (including mixed types)
        if ((leftVal == Symbols::Variables::Type::INTEGER || leftVal == Symbols::Variables::Type::FLOAT ||
             leftVal == Symbols::Variables::Type::DOUBLE) &&
            (rightVal == Symbols::Variables::Type::INTEGER || rightVal == Symbols::Variables::Type::FLOAT ||
             rightVal == Symbols::Variables::Type::DOUBLE)) {
            // Promote to double if any operand is double
            if (leftVal == Symbols::Variables::Type::DOUBLE || rightVal == Symbols::Variables::Type::DOUBLE) {
                double l = (leftVal == Symbols::Variables::Type::DOUBLE) ? leftVal->get<double>() :
                           (leftVal == Symbols::Variables::Type::FLOAT)  ? static_cast<double>(leftVal->get<float>()) :
                                                                           static_cast<double>(leftVal->get<int>());
                double r = (rightVal == Symbols::Variables::Type::DOUBLE) ? rightVal->get<double>() :
                           (rightVal == Symbols::Variables::Type::FLOAT) ? static_cast<double>(rightVal->get<float>()) :
                                                                           static_cast<double>(rightVal->get<int>());
                if (op_ == "+") {
                    return l + r;
                    //return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return l - r;
                    //return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return l * r;
                    //return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return l / r;
                    //return std::make_shared<Symbols::Value>(l / r);
                }
                if (op_ == "==") {
                    return l == r;
                    //return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return l != r;
                    //return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return l < r;
                    //return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return l > r;
                    //return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return l <= r;
                    //return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return l >= r;
                    //return std::make_shared<Symbols::Value>(l >= r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }
            // Promote to float if any operand is float
            if (leftVal == Symbols::Variables::Type::FLOAT || rightVal == Symbols::Variables::Type::FLOAT) {
                float l = (leftVal == Symbols::Variables::Type::FLOAT) ? leftVal->get<float>() :
                                                                         static_cast<float>(leftVal->get<int>());
                float r = (rightVal == Symbols::Variables::Type::FLOAT) ? rightVal->get<float>() :
                                                                          static_cast<float>(rightVal->get<int>());
                if (op_ == "+") {
                    return l + r;
                    //return std::make_shared<Symbols::Value>(l + r);
                }
                if (op_ == "-") {
                    return l - r;
                    //return std::make_shared<Symbols::Value>(l - r);
                }
                if (op_ == "*") {
                    return l * r;
                    //return std::make_shared<Symbols::Value>(l * r);
                }
                if (op_ == "/") {
                    return l / r;
                    //return std::make_shared<Symbols::Value>(l / r);
                }
                if (op_ == "==") {
                    return l == r;
                    //return std::make_shared<Symbols::Value>(l == r);
                }
                if (op_ == "!=") {
                    return l != r;
                    //return std::make_shared<Symbols::Value>(l != r);
                }
                if (op_ == "<") {
                    return l < r;
                    //return std::make_shared<Symbols::Value>(l < r);
                }
                if (op_ == ">") {
                    return l > r;
                    //return std::make_shared<Symbols::Value>(l > r);
                }
                if (op_ == "<=") {
                    return l <= r;
                    //return std::make_shared<Symbols::Value>(l <= r);
                }
                if (op_ == ">=") {
                    return l >= r;
                    //return std::make_shared<Symbols::Value>(l >= r);
                }
                throw std::runtime_error("Unknown operator: " + op_);
            }
            // Both operands are integers
            int l = leftVal;
            int r = rightVal;
            if (op_ == "+") {
                return l + r;
                //return std::make_shared<Symbols::Value>(l + r);
            }
            if (op_ == "-") {
                return l - r;
                //return std::make_shared<Symbols::Value>(l - r);
            }
            if (op_ == "*") {
                return l * r;
                //return std::make_shared<Symbols::Value>(l * r);
            }
            if (op_ == "/") {
                return l / r;
                //return std::make_shared<Symbols::Value>(l / r);
            }
            if (op_ == "%") {
                return l % r;
                //return std::make_shared<Symbols::Value>(l % r);
            }
            if (op_ == "==") {
                return l == r;
                //return std::make_shared<Symbols::Value>(l == r);
            }
            if (op_ == "!=") {
                return l != r;
                //return std::make_shared<Symbols::Value>(l != r);
            }
            if (op_ == "<") {
                return l < r;
                //return std::make_shared<Symbols::Value>(l < r);
            }
            if (op_ == ">") {
                return l > r;
                //return std::make_shared<Symbols::Value>(l > r);
            }
            if (op_ == "<=") {
                return l <= r;
                //return std::make_shared<Symbols::Value>(l <= r);
            }
            if (op_ == ">=") {
                return l >= r;
                //return std::make_shared<Symbols::Value>(l >= r);
            }
            throw std::runtime_error("Unknown operator: " + op_);
        }
        // Integer operations
        if (leftVal == Symbols::Variables::Type::INTEGER && rightVal == Symbols::Variables::Type::INTEGER) {
            int l = leftVal->get<int>();
            int r = rightVal->get<int>();

            if (op_ == "+") {
                return l + r;
                //return std::make_shared<Symbols::Value>(l + r);
            }
            if (op_ == "-") {
                return l - r;
                //return std::make_shared<Symbols::Value>(l - r);
            }
            if (op_ == "*") {
                return l * r;
                //return std::make_shared<Symbols::Value>(l * r);
            }
            if (op_ == "/") {
                return l / r;
                //return std::make_shared<Symbols::Value>(l / r);  // TODO: 0 div
            }
            if (op_ == "%") {
                return l % r;
                //return std::make_shared<Symbols::Value>(l % r);
            }
            if (op_ == "==") {
                return l == r;
                //return std::make_shared<Symbols::Value>(l == r);
            }
            if (op_ == "!=") {
                return l != r;
                //return std::make_shared<Symbols::Value>(l != r);
            }
            if (op_ == "<") {
                return l < r;
                //return std::make_shared<Symbols::Value>(l < r);
            }
            if (op_ == ">") {
                return l > r;
                //return std::make_shared<Symbols::Value>(l > r);
            }
            if (op_ == "<=") {
                return l <= r;
                //return std::make_shared<Symbols::Value>(l <= r);
            }
            if (op_ == ">=") {
                return l >= r;
                //return std::make_shared<Symbols::Value>(l >= r);
            }

            throw std::runtime_error("Unknown operator: " + op_);
        }

        // String operations
        if (leftVal == Symbols::Variables::Type::STRING && rightVal == Symbols::Variables::Type::STRING) {
            auto l = leftVal->get<std::string>();
            auto r = rightVal->get<std::string>();

            if (op_ == "+") {
                return l + r;
                //return std::make_shared<Symbols::Value>(l + r);
            }
            if (op_ == "==") {
                return l == r;
                //return std::make_shared<Symbols::Value>(l == r);
            }
            if (op_ == "!=") {
                return l != r;
                //return std::make_shared<Symbols::Value>(l != r);
            }
            throw std::runtime_error("Unknown operator: " + op_);
        }

        throw std::runtime_error(
            "Unsupported types in binary expression: " + Symbols::Variables::TypeToString(leftVal) + " and " +
            Symbols::Variables::TypeToString(rightVal) + " " + toString());

        return Symbols::ValuePtr::null();
    };

    std::string toString() const override { return "(" + lhs_->toString() + " " + op_ + " " + rhs_->toString() + ")"; }
};  // class
}  // namespace Interpreter
