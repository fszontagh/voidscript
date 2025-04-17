#ifndef PARSEREXPRESSION_BUILDER_HPP
#define PARSEREXPRESSION_BUILDER_HPP

#include <memory>
#include <stdexcept>

#include "Interpreter/BinaryExpressionNode.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/IdentifierExpressionNode.hpp"
#include "Interpreter/LiteralExpressionNode.hpp"
#include "Interpreter/UnaryExpressionNode.hpp" // <-- új include
#include "Parser/ParsedExpression.hpp"

namespace Parser {
static std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(
    const Parser::ParsedExpressionPtr & expr) {
    using Kind = Parser::ParsedExpression::Kind;

    switch (expr->kind) {
        case Kind::Literal:
            return std::make_unique<Interpreter::LiteralExpressionNode>(expr->value);

        case Kind::Variable:
            return std::make_unique<Interpreter::IdentifierExpressionNode>(expr->name);

        case Kind::Binary: {
            auto lhs = buildExpressionFromParsed(expr->lhs);
            auto rhs = buildExpressionFromParsed(expr->rhs);
            return std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), expr->op, std::move(rhs));
        }

        case Kind::Unary: {
            auto operand = buildExpressionFromParsed(expr->rhs);  // rhs az operandus
            return std::make_unique<Interpreter::UnaryExpressionNode>(expr->op, std::move(operand));
        }
    }

    throw std::runtime_error("Unknown ParsedExpression kind");
}
}  // namespace Parser

#endif  // PARSEREXPRESSION_BUILDER_HPP
