#ifndef PARSEREXPRESSION_BUILDER_HPP
#define PARSEREXPRESSION_BUILDER_HPP

#include <memory>
#include <stdexcept>

#include "Interpreter/ExpressionNode.hpp"
#include "Nodes/Expression/ArrayAccessExpressionNode.hpp"
#include "Nodes/Expression/BinaryExpressionNode.hpp"
#include "Nodes/Expression/CallExpressionNode.hpp"
#include "Nodes/Expression/DynamicMemberExpressionNode.hpp"
#include "Nodes/Expression/IdentifierExpressionNode.hpp"
#include "Nodes/Expression/LiteralExpressionNode.hpp"
#include "Nodes/Expression/MemberExpressionNode.hpp"
#include "Nodes/Expression/MethodCallExpressionNode.hpp"
#include "Nodes/Expression/NewExpressionNode.hpp"
#include "Nodes/Expression/ObjectExpressionNode.hpp"
#include "Nodes/Expression/UnaryExpressionNode.hpp"
#include "Parser/ParsedExpression.hpp"

namespace Parser {

inline std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(const ParsedExpressionPtr & expr) {
    using Kind = ParsedExpression::Kind;

    // Explicitly map ParsedExpression kinds to ExpressionNode types
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
                        std::move(arrExpr), std::move(idxExpr), expr->filename, expr->line, expr->column);
                }
                // Member access: '->' (never method call here)
                if (expr->op == "->") {
                    auto objectExpr = buildExpressionFromParsed(expr->lhs);
                    if (expr->rhs->kind == ParsedExpression::Kind::Call || expr->kind == ParsedExpression::Kind::MethodCall) {
                        // Handle both object->method() syntax paths
                        std::vector<std::unique_ptr<Interpreter::ExpressionNode>> callArgs;
                        const auto& argsToUse = expr->kind == ParsedExpression::Kind::MethodCall ? expr->args : expr->rhs->args;
                        callArgs.reserve(argsToUse.size());
                        for (const auto & arg : argsToUse) {
                            callArgs.push_back(buildExpressionFromParsed(arg));
                        }
                        const std::string& methodName = expr->kind == ParsedExpression::Kind::MethodCall ? expr->name : expr->rhs->name;
                        return std::make_unique<Interpreter::MethodCallExpressionNode>(
                            std::move(objectExpr), methodName, std::move(callArgs), 
                            expr->filename, expr->line, expr->column);
                    } else if (expr->rhs->kind == ParsedExpression::Kind::Literal) {
                        std::string propName = expr->rhs->value->get<std::string>();
                        if (propName.size() > 3 && propName.substr(0, 2) == "${" && propName.back() == '}') {
                            return std::make_unique<Interpreter::DynamicMemberExpressionNode>(
                                std::move(objectExpr),
                                std::make_unique<Interpreter::IdentifierExpressionNode>(propName), expr->filename,
                                expr->line, expr->column);
                        }
                        return std::make_unique<Interpreter::MemberExpressionNode>(
                            std::move(objectExpr), propName, expr->filename, expr->line, expr->column);
                    }
                    if (expr->rhs->kind == ParsedExpression::Kind::Variable) {
                        return std::make_unique<Interpreter::MemberExpressionNode>(
                            std::move(objectExpr), expr->rhs->name, expr->filename, expr->line, expr->column);
                    }
                    std::string msg = "Invalid member access expression - right side has unexpected kind: " +
                                      ParsedExpression::kindToString(expr->rhs->kind);
                    throw Interpreter::Exception(msg, expr->filename, expr->line, expr->column);
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
        case Kind::MethodCall:
            {
                // Only method calls (obj->method(args)) produce this node
                std::vector<std::unique_ptr<Interpreter::ExpressionNode>> callArgs;
                callArgs.reserve(expr->args.size());
                for (const auto & arg : expr->args) {
                    callArgs.push_back(buildExpressionFromParsed(arg));
                }
                return std::make_unique<Interpreter::MethodCallExpressionNode>(
                    buildExpressionFromParsed(expr->lhs), expr->name, std::move(callArgs), expr->filename, expr->line,
                    expr->column);
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
                auto        objectExpr = buildExpressionFromParsed(expr->objectMembers[0].second);
                std::string propName   = expr->objectMembers[0].first;

                return std::make_unique<Interpreter::MemberExpressionNode>(std::move(objectExpr), propName,
                                                                           expr->filename, expr->line, expr->column);
            }
        default:
            {
                throw std::runtime_error("Unknown ParsedExpression kind: " +
                                         ParsedExpression::kindToString(expr->kind));
            }
            break;
    }

    throw std::runtime_error("Unknown ParsedExpression kind: " + ParsedExpression::kindToString(expr->kind));
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

                // Check if we're dealing with a comparison operator
                bool isComparison = (expr->op == "==" || expr->op == "!=" ||
                                   expr->op == "<" || expr->op == ">" ||
                                   expr->op == "<=" || expr->op == ">=");

                // For comparisons, both operands must be numeric or both string
                if (isComparison) {
                    bool lhsNumeric = (lhsType == Symbols::Variables::Type::INTEGER ||
                                     lhsType == Symbols::Variables::Type::FLOAT ||
                                     lhsType == Symbols::Variables::Type::DOUBLE);
                    bool rhsNumeric = (rhsType == Symbols::Variables::Type::INTEGER ||
                                     rhsType == Symbols::Variables::Type::FLOAT ||
                                     rhsType == Symbols::Variables::Type::DOUBLE);
                    
                    if (lhsNumeric != rhsNumeric) {
                        throw std::runtime_error("Type mismatch in comparison: " +
                            Symbols::Variables::TypeToString(lhsType) + " " + expr->op + " " +
                            Symbols::Variables::TypeToString(rhsType));
                    }
                }
                // For arithmetic operators
                else if (expr->op == "+" || expr->op == "-" || expr->op == "*" || expr->op == "/") {
                    bool lhsNumeric = (lhsType == Symbols::Variables::Type::INTEGER ||
                                     lhsType == Symbols::Variables::Type::FLOAT ||
                                     lhsType == Symbols::Variables::Type::DOUBLE);
                    bool rhsNumeric = (rhsType == Symbols::Variables::Type::INTEGER ||
                                     rhsType == Symbols::Variables::Type::FLOAT ||
                                     rhsType == Symbols::Variables::Type::DOUBLE);
                    
                    if (!lhsNumeric || !rhsNumeric) {
                        throw std::runtime_error("Operands must be numeric for operator: " + expr->op);
                    }
                }
                // For logical operators, both operands must be boolean
                else if (expr->op == "&&" || expr->op == "||") {
                    if (lhsType != Symbols::Variables::Type::BOOLEAN ||
                        rhsType != Symbols::Variables::Type::BOOLEAN) {
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
