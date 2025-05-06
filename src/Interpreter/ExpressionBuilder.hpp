#ifndef PARSEREXPRESSION_BUILDER_HPP
#define PARSEREXPRESSION_BUILDER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>

#include "Interpreter/ExpressionNode.hpp"
#include "Nodes/Expression/ArrayAccessExpressionNode.hpp"
#include "Nodes/Expression/BinaryExpressionNode.hpp"
#include "Nodes/Expression/CallExpressionNode.hpp"
#include "Nodes/Expression/IdentifierExpressionNode.hpp"
#include "Nodes/Expression/LiteralExpressionNode.hpp"
#include "Nodes/Expression/MemberExpressionNode.hpp"
#include "Nodes/Expression/DynamicMemberExpressionNode.hpp"
#include "Nodes/Expression/MethodCallExpressionNode.hpp"
#include "Nodes/Expression/NewExpressionNode.hpp"
#include "Nodes/Expression/ObjectExpressionNode.hpp"
#include "Nodes/Expression/UnaryExpressionNode.hpp"
#include "Parser/ParsedExpression.hpp"

namespace Parser {


inline std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(const ParsedExpressionPtr & expr) {
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
                    return std::make_unique<Interpreter::ArrayAccessExpressionNode>(
                        std::move(arrExpr), std::move(idxExpr),
                        expr->filename, expr->line, expr->column);
                }
                // Member access or method invocation: '->'
                if (expr->op == "->") {
                    auto objectExpr = buildExpressionFromParsed(expr->lhs);
                    // Method call: rhs is a parsed call expression
                    if (expr->rhs->kind == ParsedExpression::Kind::Call) {
                        std::vector<std::unique_ptr<Interpreter::ExpressionNode>> methodArgs;
                        methodArgs.reserve(expr->rhs->args.size());
                        for (const auto & arg : expr->rhs->args) {
                            methodArgs.push_back(buildExpressionFromParsed(arg));
                        }
                        return std::make_unique<Interpreter::MethodCallExpressionNode>(
                            std::move(objectExpr), expr->rhs->name, std::move(methodArgs),
                            expr->filename, expr->line, expr->column);
                    }
                    // Property access on object
                    else if (expr->rhs->kind == ParsedExpression::Kind::Literal) {
                        // Static member name (string literal or identifier)
                        std::string propName = expr->rhs->value.get<std::string>();
                        // Check if it's a dynamic access marker
                        if (propName.size() > 3 && propName.substr(0, 2) == "${" && propName.back() == '}') {
                            // For dynamic access, we'll create a special marker in the property path
                            // that will be handled by the expression builder
                            return std::make_unique<Interpreter::DynamicMemberExpressionNode>(
                                std::move(objectExpr), std::make_unique<Interpreter::IdentifierExpressionNode>(propName),
                                expr->filename, expr->line, expr->column);
                        }
                        // Regular static member access
                        return std::make_unique<Interpreter::MemberExpressionNode>(
                            std::move(objectExpr), propName,
                            expr->filename, expr->line, expr->column);
                    }
                    else if (expr->rhs->kind == ParsedExpression::Kind::Variable) {
                        // Variable as property name - convert to static member access
                        return std::make_unique<Interpreter::MemberExpressionNode>(
                            std::move(objectExpr), expr->rhs->name,
                            expr->filename, expr->line, expr->column);
                    }
                    else {
                        std::string msg = "Invalid member access expression - right side has unexpected kind: ";
                        switch (expr->rhs->kind) {
                            case ParsedExpression::Kind::Binary: msg += "Binary"; break;
                            case ParsedExpression::Kind::Unary: msg += "Unary"; break;
                            case ParsedExpression::Kind::MethodCall: msg += "MethodCall"; break;
                            case ParsedExpression::Kind::New: msg += "New"; break;
                            case ParsedExpression::Kind::Object: msg += "Object"; break;
                            default: msg += "Unknown";
                        }
                        throw Interpreter::Exception(msg, expr->filename, expr->line, expr->column);
                    }
                }
                // Default binary operator
                auto lhs = buildExpressionFromParsed(expr->lhs);
                auto rhs = buildExpressionFromParsed(expr->rhs);
                return std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), expr->op, std::move(rhs));
            }

        case Kind::Unary:
            {
                auto operand = buildExpressionFromParsed(expr->rhs);
                return std::make_unique<Interpreter::UnaryExpressionNode>(expr->op, std::move(operand));
            }

        case Kind::Call:
            {
                std::vector<std::unique_ptr<Interpreter::ExpressionNode>> callArgs;
                callArgs.reserve(expr->args.size());
                for (const auto & arg : expr->args) {
                    callArgs.push_back(buildExpressionFromParsed(arg));
                }
                return std::make_unique<Interpreter::CallExpressionNode>(expr->name, std::move(callArgs),
                                                                         expr->filename, expr->line, expr->column);
            }

        case Kind::Object:
            {
                std::vector<std::pair<std::string, std::unique_ptr<Interpreter::ExpressionNode>>> members;
                members.reserve(expr->objectMembers.size());
                for (const auto & p : expr->objectMembers) {
                    members.emplace_back(p.first, buildExpressionFromParsed(p.second));
                }
                return std::make_unique<Interpreter::ObjectExpressionNode>(std::move(members));
            }

        case Kind::New:
            {
                std::vector<std::unique_ptr<Interpreter::ExpressionNode>> ctorArgs;
                ctorArgs.reserve(expr->args.size());
                for (const auto & arg : expr->args) {
                    ctorArgs.push_back(buildExpressionFromParsed(arg));
                }
                return std::make_unique<Interpreter::NewExpressionNode>(expr->name, std::move(ctorArgs), expr->filename,
                                                                        expr->line, expr->column);
            }

        case Kind::Member:
            {
                // Handle member access expressions created using makeMember
                // Get the object and property name from the first entry in objectMembers
                auto objectExpr = buildExpressionFromParsed(expr->objectMembers[0].second);
                std::string propName = expr->objectMembers[0].first;
                
                return std::make_unique<Interpreter::MemberExpressionNode>(
                    std::move(objectExpr), propName,
                    expr->filename, expr->line, expr->column);
            }
    }

    throw std::runtime_error("Unknown ParsedExpression kind");
}

inline void typecheckParsedExpression(const ParsedExpressionPtr & expr) {
    using Kind = ParsedExpression::Kind;

    switch (expr->kind) {
        case Kind::Literal:
            {
                break;
            }

        case Kind::Variable:
            {
                // Lookup variable using findSymbol, which handles scope hierarchy and sub-namespaces correctly.
                auto symbol = Symbols::SymbolContainer::instance()->findSymbol(expr->name);
                if (!symbol) {
                    // If not found by findSymbol, it could be a function name used as a variable, which is an error.
                    // Or it's genuinely not found.
                    // findSymbol checks both ::variables and constants.
                    throw std::runtime_error("Symbol not found or cannot be used as a variable: " + expr->name);
                }

                // Check if the found symbol is a variable or constant (findSymbol checks both)
                // If it's a function, it's an error because findSymbol doesn't return functions for this usage.
                // This check is implicitly handled by findSymbol only looking in variable/constant namespaces.
                // However, an explicit check might be desired if findSymbol's behavior changes.
                // For now, if findSymbol returns something, it's usable as a variable/constant value.

                // Ha a szimbólum nem egy változó, akkor hibát dobunk (Original Comment)
                // This original check might be redundant if findSymbol is used correctly.
                // if (symbol->getKind() == Symbols::Kind::Function) {
                // throw std::runtime_error("Cannot use function as variable: " + expr->name);
                // }
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
