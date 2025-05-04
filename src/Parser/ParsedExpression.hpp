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

using ParsedExpressionPtr = std::shared_ptr<ParsedExpression>;

struct ParsedExpression {
    enum class Kind : std::uint8_t { Literal, Variable, Binary, Unary, Call, MethodCall, New, Object, Member };

    Kind kind;

    Symbols::Value value;
    std::string    name;

    // For operations
    std::string         op;
    ParsedExpressionPtr lhs;
    ParsedExpressionPtr rhs;
    // For function call arguments
    std::vector<ParsedExpressionPtr> args;
    std::vector<std::pair<std::string, ParsedExpressionPtr>> objectMembers;
    // Source location for error reporting
    std::string filename;
    int line = 0;
    size_t column = 0;

    // Constructor for literal
    static ParsedExpressionPtr makeLiteral(const Symbols::Value & val) {
        auto expr   = std::make_shared<ParsedExpression>();
        expr->kind  = Kind::Literal;
        expr->value = val;
        return expr;
    }

    // Constructor for variable
    static ParsedExpressionPtr makeVariable(const std::string & name) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Variable;
        expr->name = name;
        return expr;
    }

    // Constructor for binary operation
    static ParsedExpressionPtr makeBinary(std::string op, ParsedExpressionPtr left, ParsedExpressionPtr right) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Binary;
        expr->op   = std::move(op);
        expr->lhs  = std::move(left);
        expr->rhs  = std::move(right);
        return expr;
    }

    // Constructor for unary operation
    static ParsedExpressionPtr makeUnary(std::string op, ParsedExpressionPtr operand) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Unary;
        expr->op   = std::move(op);
        expr->rhs  = std::move(operand);
        return expr;
    }
    // Constructor for function call
    static ParsedExpressionPtr makeCall(const std::string &name, std::vector<ParsedExpressionPtr> arguments) {
        auto expr        = std::make_shared<ParsedExpression>();
        expr->kind       = Kind::Call;
        expr->name       = name;
        expr->args       = std::move(arguments);
        return expr;
    }
    
    // Constructor for method call: object->method(args)
    static ParsedExpressionPtr makeMethodCall(ParsedExpressionPtr object, const std::string &methodName,
                                              std::vector<ParsedExpressionPtr> arguments) {
        auto expr         = std::make_shared<ParsedExpression>();
        expr->kind        = Kind::MethodCall;
        expr->lhs         = std::move(object);
        expr->name        = methodName;
        expr->args        = std::move(arguments);
        return expr;
    }

    // Constructor for 'new' expression: instantiate class
    static ParsedExpressionPtr makeNew(const std::string &className, std::vector<ParsedExpressionPtr> arguments) {
        auto expr        = std::make_shared<ParsedExpression>();
        expr->kind       = Kind::New;
        expr->name       = className;
        expr->args       = std::move(arguments);
        return expr;
    }
    // Constructor for object literal
    static ParsedExpressionPtr makeObject(std::vector<std::pair<std::string, ParsedExpressionPtr>> members) {
        auto expr = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Object;
        expr->objectMembers = std::move(members);
        return expr;
    }

    static ParsedExpressionPtr makeMember(ParsedExpressionPtr object, const std::string &propName) {
        auto expr = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Member;
        expr->objectMembers.push_back({propName, std::move(object)});
        return expr;
    }

    Symbols::Variables::Type getType() const {
        switch (kind) {
            case Kind::Literal:
                return value.getType();
                break;

            case Kind::Variable:
                {
                    // Lookup variable in current scope's variables namespace
                    const auto ns     = Symbols::SymbolContainer::instance()->currentScopeName() + "::variables";
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
                    // Lookup function in current scope's functions namespace
                    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName() + "::functions";
                    auto symbol = Symbols::SymbolContainer::instance()->get(ns, name);
                    if (!symbol) {
                        throw std::runtime_error("Unknown function: " + name + " in namespace: " + ns);
                    }
                    // FunctionSymbol holds return type
                    auto funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(symbol);
                    return funcSym->returnType();
                }
            case Kind::Object:
                return Symbols::Variables::Type::OBJECT;

            default:
                throw std::runtime_error("Unknown expression kind");
        }

        throw std::runtime_error("Could not determine type for expression");
    }

    std::string toString() const {
        switch (kind) {
            case Kind::Literal:
                return Symbols::Value::to_string(value);
            case Kind::Variable:
                return name;
            case Kind::Binary:
                return "(" + lhs->toString() + " " + op + " " + rhs->toString() + ")";
            case Kind::Unary:
                return "(" + op + rhs->toString() + ")";
            case Kind::Call:
                {
                    std::string result = name + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += args[i]->toString();
                    }
                    result += ")";
                    return result;
                }
            case Kind::MethodCall:
                {
                    std::string result = lhs->toString() + "->" + name + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += args[i]->toString();
                    }
                    result += ")";
                    return result;
                }
            case Kind::Object:
                {
                    std::string result = "{";
                    for (size_t i = 0; i < objectMembers.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += objectMembers[i].first + ": " + objectMembers[i].second->toString();
                    }
                    result += "}";
                    return result;
                }
            case Kind::New:
                {
                    std::string result = "new " + name + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += args[i]->toString();
                    }
                    result += ")";
                    return result;
                }
            case Kind::Member:
                return objectMembers[0].second->toString() + "->" + objectMembers[0].first;
            default:
                return "Unknown expression kind";
        }
    }
};

}  // namespace Parser

#endif  // PARSEDEXPRESSION_HPP
