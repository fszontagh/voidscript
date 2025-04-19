#ifndef PARSEREXPRESSION_BUILDER_HPP
#define PARSEREXPRESSION_BUILDER_HPP

#include <memory>
#include <stdexcept>

#include "Interpreter/BinaryExpressionNode.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/IdentifierExpressionNode.hpp"
#include "Interpreter/LiteralExpressionNode.hpp"
#include "Interpreter/UnaryExpressionNode.hpp"  // <-- új include
#include "Interpreter/CallExpressionNode.hpp"
#include "Interpreter/MemberExpressionNode.hpp"
#include "Interpreter/ArrayAccessExpressionNode.hpp"
#include "Interpreter/ObjectExpressionNode.hpp"
#include "Interpreter/ObjectExpressionNode.hpp"
#include "Parser/ParsedExpression.hpp"

namespace Parser {
static std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(
    const ParsedExpressionPtr & expr) {
    using Kind = ParsedExpression::Kind;

    switch (expr->kind) {
        case Kind::Literal:
            return std::make_unique<Interpreter::LiteralExpressionNode>(expr->value);

        case Kind::Variable:
            return std::make_unique<Interpreter::IdentifierExpressionNode>(expr->name);

        case Kind::Binary:
            {
                // Array/object dynamic indexing: operator []
                if (expr->op == "[]") {
                    auto arrExpr = buildExpressionFromParsed(expr->lhs);
                    auto idxExpr = buildExpressionFromParsed(expr->rhs);
                    return std::make_unique<Interpreter::ArrayAccessExpressionNode>(std::move(arrExpr), std::move(idxExpr));
                }
                // Member access for object properties: '->'
                if (expr->op == "->") {
                    auto objExpr = buildExpressionFromParsed(expr->lhs);
                    std::string propName;
                    if (expr->rhs->kind == ParsedExpression::Kind::Literal &&
                        expr->rhs->value.getType() == Symbols::Variables::Type::STRING) {
                        propName = expr->rhs->value.get<std::string>();
                    } else if (expr->rhs->kind == ParsedExpression::Kind::Variable) {
                        propName = expr->rhs->name;
                    } else {
                        throw std::runtime_error("Invalid property name in member access");
                    }
                    return std::make_unique<Interpreter::MemberExpressionNode>(std::move(objExpr), propName);
                }
                // Default binary operator
                auto lhs = buildExpressionFromParsed(expr->lhs);
                auto rhs = buildExpressionFromParsed(expr->rhs);
                return std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), expr->op, std::move(rhs));
            }

        case Kind::Unary:
            {
                auto operand = buildExpressionFromParsed(expr->rhs);  // rhs az operandus
                return std::make_unique<Interpreter::UnaryExpressionNode>(expr->op, std::move(operand));
            }
        case Kind::Call:
            {
                // Build argument expressions
                std::vector<std::unique_ptr<Interpreter::ExpressionNode>> callArgs;
                callArgs.reserve(expr->args.size());
                for (const auto &arg : expr->args) {
                    callArgs.push_back(buildExpressionFromParsed(arg));
                }
                return std::make_unique<Interpreter::CallExpressionNode>(expr->name, std::move(callArgs));
            }
        case Kind::Object:
            {
                std::vector<std::pair<std::string, std::unique_ptr<Interpreter::ExpressionNode>>> members;
                members.reserve(expr->objectMembers.size());
                for (const auto &p : expr->objectMembers) {
                    members.emplace_back(p.first, buildExpressionFromParsed(p.second));
                }
                return std::make_unique<Interpreter::ObjectExpressionNode>(std::move(members));
            }
    }

    throw std::runtime_error("Unknown ParsedExpression kind");
}

void typecheckParsedExpression(const ParsedExpressionPtr & expr) {
    using Kind = ParsedExpression::Kind;

    switch (expr->kind) {
        case Kind::Literal:
            {
                // Literál típusának ellenőrzése - a literál típusát a value.getType() adja vissza
                // auto type = expr->value.getType();
                // Nem szükséges semmilyen más típusellenőrzés a literálokhoz, mivel azok fix típusúak.
                break;
            }

        case Kind::Variable:
            {
                const std::string ns     = Symbols::SymbolContainer::instance()->currentScopeName() + ".variables";
                auto              symbol = Symbols::SymbolContainer::instance()->get(ns, expr->name);
                if (!symbol) {
                    throw std::runtime_error("Variable not found in symbol table: " + expr->name);
                }

                // Ha a szimbólum nem egy változó, akkor hibát dobunk
                if (symbol->getKind() == Symbols::Kind::Function) {
                    throw std::runtime_error("Cannot use function as variable: " + expr->name);
                }
                break;
            }

        case Kind::Binary:
            {
                // Bináris kifejezés operandusainak típusellenőrzése
                typecheckParsedExpression(expr->lhs);
                typecheckParsedExpression(expr->rhs);

                auto lhsType = expr->lhs->getType();
                auto rhsType = expr->rhs->getType();

                if (lhsType != rhsType) {
                    throw std::runtime_error(
                        "Type mismatch in binary expression: " + Symbols::Variables::TypeToString(lhsType) + " and " +
                        Symbols::Variables::TypeToString(rhsType));
                }

                // Bináris operátoroknál is elvégezhetjük a típusellenőrzést:
                // Ha numerikus operátor, akkor az operandusoknak numerikusnak kell lenniük
                if (expr->op == "+" || expr->op == "-" || expr->op == "*" || expr->op == "/") {
                    if (lhsType != Symbols::Variables::Type::INTEGER && lhsType != Symbols::Variables::Type::FLOAT) {
                        throw std::runtime_error("Operands must be numeric for operator: " + expr->op);
                    }
                }
                // Ha logikai operátorok, akkor boolean típus szükséges
                else if (expr->op == "&&" || expr->op == "||") {
                    if (lhsType != Symbols::Variables::Type::BOOLEAN) {
                        throw std::runtime_error("Operands must be boolean for operator: " + expr->op);
                    }
                }
                break;
            }

        case Kind::Unary:
            {
                // Unáris kifejezés operandusának típusellenőrzése
                typecheckParsedExpression(expr->rhs);  // 'rhs' tárolja az operandust az unáris kifejezésnél

                auto operandType = expr->rhs->getType();

                if (expr->op == "!") {
                    if (operandType != Symbols::Variables::Type::BOOLEAN) {
                        throw std::runtime_error("Operand must be boolean for unary operator '!'");
                    }
                }
                break;
            }

        default:
            throw std::runtime_error("Unknown expression kind");
    }
}

}  // namespace Parser

#endif  // PARSEREXPRESSION_BUILDER_HPP
