#include <iostream> // For std::cerr, std::endl
#include <memory>
#include <sstream>  // For std::stringstream
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp" // Required for ValuePtr and TypeToString

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
        if (leftVal->is_null() || rightVal->is_null()) {
            if (op_ == "==") {
                return leftVal->is_null() == rightVal->is_null();
            }
            if (op_ == "!=") {
                return leftVal->is_null() != rightVal->is_null();
            }
            return false;
        }

        // Boolean operations
        if (leftVal == Symbols::Variables::Type::BOOLEAN && rightVal == Symbols::Variables::Type::BOOLEAN) {
            bool l = leftVal.toBool();
            bool r = rightVal.toBool();
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
        // Bitwise operations. Integer-only by design: applying them to a float would
        // require reinterpreting its bit pattern, which is never what a script means.
        if (op_ == "&" || op_ == "|" || op_ == "^" || op_ == "<<" || op_ == ">>") {
            if (leftVal != Symbols::Variables::Type::INTEGER || rightVal != Symbols::Variables::Type::INTEGER) {
                throw std::runtime_error("Bitwise operator '" + op_ + "' requires integer operands, got " +
                                         Symbols::Variables::TypeToString(leftVal) + " and " +
                                         Symbols::Variables::TypeToString(rightVal));
            }
            const int l = leftVal->get<int>();
            const int r = rightVal->get<int>();
            if (op_ == "&") {
                return Symbols::ValuePtr(l & r);
            }
            if (op_ == "|") {
                return Symbols::ValuePtr(l | r);
            }
            if (op_ == "^") {
                return Symbols::ValuePtr(l ^ r);
            }
            // Shifting by a negative amount or by >= the operand width is undefined
            // behaviour in C++, so reject it rather than let it through.
            if (r < 0 || r >= static_cast<int>(sizeof(int) * 8)) {
                throw std::runtime_error("Shift amount " + std::to_string(r) + " is out of range for '" + op_ + "'");
            }
            return Symbols::ValuePtr(op_ == "<<" ? (l << r) : (l >> r));
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

        // Mixed string concatenation: "count=" + $n. '+' already means concatenation
        // when both sides are strings, so extending it to "string on exactly one side"
        // is consistent, and it is by far the most natural way to build a message.
        // Deliberately limited to '+': comparing a string with a number stays a type
        // error rather than silently stringifying one side.
        if (op_ == "+" && (leftVal == Symbols::Variables::Type::STRING ||
                           rightVal == Symbols::Variables::Type::STRING)) {
            return leftVal->toString() + rightVal->toString();
        }

        throw std::runtime_error(
            "Unsupported types in binary expression: " + Symbols::Variables::TypeToString(leftVal) + " and " +
            Symbols::Variables::TypeToString(rightVal) + " " + toString());

        return Symbols::ValuePtr::null();
    };

    std::string toString() const override { return "(" + lhs_->toString() + " " + op_ + " " + rhs_->toString() + ")"; }
};  // class
}  // namespace Interpreter
