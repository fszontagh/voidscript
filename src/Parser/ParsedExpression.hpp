#ifndef PARSEDEXPRESSION_HPP
#define PARSEDEXPRESSION_HPP

#include <memory>
#include <string>
#include <vector>

#include "../Symbols/SymbolContainer.hpp"
#include "../Symbols/Value.hpp"
#include "../Symbols/FunctionSymbol.hpp"

namespace Parser {

struct ParsedExpression;

using ParsedExpressionPtr = std::unique_ptr<ParsedExpression>;

struct ParsedExpression {
    enum class Kind : std::uint8_t { Literal, Variable, Binary, Unary, Call };

    Kind kind;

    Symbols::Value value;
    std::string    name;

    // For operations
    std::string         op;
    ParsedExpressionPtr lhs;
    ParsedExpressionPtr rhs;
    // For function call arguments
    std::vector<ParsedExpressionPtr> args;

    // Constructor for literal
    static ParsedExpressionPtr makeLiteral(const Symbols::Value & val) {
        auto expr   = std::make_unique<ParsedExpression>();
        expr->kind  = Kind::Literal;
        expr->value = val;
        return expr;
    }

    // Constructor for variable
    static ParsedExpressionPtr makeVariable(const std::string & name) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Variable;
        expr->name = name;
        return expr;
    }

    // Constructor for binary operation
    static ParsedExpressionPtr makeBinary(std::string op, ParsedExpressionPtr left, ParsedExpressionPtr right) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Binary;
        expr->op   = std::move(op);
        expr->lhs  = std::move(left);
        expr->rhs  = std::move(right);
        return expr;
    }

    // Constructor for unary operation
    static ParsedExpressionPtr makeUnary(std::string op, ParsedExpressionPtr operand) {
        auto expr  = std::make_unique<ParsedExpression>();
        expr->kind = Kind::Unary;
        expr->op   = std::move(op);
        expr->rhs  = std::move(operand);
        return expr;
    }
    // Constructor for function call
    static ParsedExpressionPtr makeCall(const std::string &name, std::vector<ParsedExpressionPtr> arguments) {
        auto expr        = std::make_unique<ParsedExpression>();
        expr->kind       = Kind::Call;
        expr->name       = name;
        expr->args       = std::move(arguments);
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
                    return lhsType;  // In binary expressions, operand types match, so we can return the left-hand type
                }

            case Kind::Unary:
                {
                    //auto operandType = op.
                    if (op == "!") {
                        return Symbols::Variables::Type::BOOLEAN;  // Because the '!' operator expects a boolean type
                    }
                    break;
                }
            case Kind::Call:
                {
                    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName() + ".functions";
                    auto symbol = Symbols::SymbolContainer::instance()->get(ns, name);
                    if (!symbol) {
                        throw std::runtime_error("Unknown function: " + name + " in namespace: " + ns);
                    }
                    // FunctionSymbol holds return type
                    auto funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(symbol);
                    return funcSym->returnType();
                }

            default:
                throw std::runtime_error("Unknown expression kind");
        }

        throw std::runtime_error("Could not determine type for expression");
    }
};

}  // namespace Parser

#endif  // PARSEDEXPRESSION_HPP
