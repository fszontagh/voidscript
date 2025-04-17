#ifndef PARSEDEXPRESSION_HPP
#define PARSEDEXPRESSION_HPP

#include <memory>
#include <string>

#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Parser {

struct ParsedExpression;

using ParsedExpressionPtr = std::unique_ptr<ParsedExpression>;

struct ParsedExpression {
    enum class Kind : std::uint8_t { Literal, Variable, Binary, Unary };

    Kind kind;

    Symbols::Value value;
    std::string    name;

    // Műveletekhez
    std::string         op;
    ParsedExpressionPtr lhs;
    ParsedExpressionPtr rhs;

    // Konstruktor literálhoz
    static ParsedExpressionPtr makeLiteral(const Symbols::Value & val) {
        auto expr   = std::make_unique<ParsedExpression>();
        expr->kind  = Kind::Literal;
        expr->value = val;
        return expr;
    }

    // Konstruktor változóhoz
    static ParsedExpressionPtr makeVariable(const std::string & name) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Variable;
        expr->name = name;
        return expr;
    }

    // Konstruktor binárishoz
    static ParsedExpressionPtr makeBinary(std::string op, ParsedExpressionPtr left, ParsedExpressionPtr right) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Binary;
        expr->op   = std::move(op);
        expr->lhs  = std::move(left);
        expr->rhs  = std::move(right);
        return expr;
    }

    // Konstruktor unárishoz
    static ParsedExpressionPtr makeUnary(std::string op, ParsedExpressionPtr operand) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Unary;
        expr->op   = std::move(op);
        expr->rhs  = std::move(operand);
        return expr;
    }

    Symbols::Variables::Type getType() const {
        switch (kind) {
            case Kind::Literal:
                return value.getType();
                break;

            case Kind::Variable:
                {
                    const auto ns     = Symbols::SymbolContainer::instance()->currentScopeName() + ".variables";
                    auto       symbol = Symbols::SymbolContainer::instance()->get(ns, name);
                    if (!symbol) {
                        throw std::runtime_error("Unknown variable: " + name + " in namespace: " + ns +
                                                 " File: " + __FILE__ + ":" + std::to_string(__LINE__));
                    }
                    return symbol->getValue().getType();
                }

            case Kind::Binary:
                {
                    auto lhsType = lhs->value.getType();
                    //auto rhsType = rhs->value.getType();
                    return lhsType;  // Bináris kifejezésnél a típusok azonosak, tehát a bal oldali típust visszaadhatjuk
                }

            case Kind::Unary:
                {
                    //auto operandType = op.
                    if (op == "!") {
                        return Symbols::Variables::Type::BOOLEAN;  // Mivel a '!' operátor bool típust vár
                    }
                    break;
                }

            default:
                throw std::runtime_error("Unknown expression kind");
        }

        throw std::runtime_error("Could not determine type for expression");
    }
};

}  // namespace Parser

#endif  // PARSEDEXPRESSION_HPP
