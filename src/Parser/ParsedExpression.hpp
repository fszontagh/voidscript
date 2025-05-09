#ifndef PARSEDEXPRESSION_HPP
#define PARSEDEXPRESSION_HPP

#include <memory>
#include <string>
#include <vector>

#include "../Symbols/SymbolContainer.hpp"
#include "../Symbols/Value.hpp"
#include "../Symbols/FunctionSymbol.hpp"

// Forward declarations
namespace Modules {
    class UnifiedModuleManager;
}

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
    static ParsedExpressionPtr makeBinary(std::string op, ParsedExpressionPtr left, ParsedExpressionPtr right, const std::string &filename, int line, size_t column) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Binary;
        expr->op   = std::move(op);
        expr->lhs  = std::move(left);
        expr->rhs  = std::move(right);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    // Constructor for unary operation
    static ParsedExpressionPtr makeUnary(std::string op, ParsedExpressionPtr operand, const std::string &filename, int line, size_t column) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Unary;
        expr->op   = std::move(op);
        expr->rhs  = std::move(operand);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;
        
        return expr;
    }
    // Constructor for function call
    static ParsedExpressionPtr makeCall(const std::string &name, std::vector<ParsedExpressionPtr> arguments, const std::string &filename, int line, size_t column) {
        auto expr        = std::make_shared<ParsedExpression>();
        expr->kind       = Kind::Call;
        expr->name       = name;
        expr->args       = std::move(arguments);
        expr->filename   = filename;
        expr->line       = line;
        expr->column     = column;
        
        return expr;
    }
    
    // Constructor for method call: object->method(args)
    static ParsedExpressionPtr makeMethodCall(ParsedExpressionPtr object, const std::string &methodName,
                                              std::vector<ParsedExpressionPtr> arguments, const std::string &filename, int line, size_t column) {
        auto expr         = std::make_shared<ParsedExpression>();
        expr->kind        = Kind::MethodCall;
        expr->lhs         = std::move(object);
        expr->name        = methodName;
        expr->args        = std::move(arguments);
        expr->filename    = filename;
        expr->line        = line;
        expr->column      = column;

        return expr;
    }

    // Constructor for 'new' expression: instantiate class
    static ParsedExpressionPtr makeNew(const std::string &className, std::vector<ParsedExpressionPtr> arguments, const std::string &filename, int line, size_t column) {
        auto expr        = std::make_shared<ParsedExpression>();
        expr->kind       = Kind::New;
        expr->name       = className;
        expr->args       = std::move(arguments);
        expr->filename    = filename;
        expr->line        = line;
        expr->column      = column;

        return expr;
    }
    // Constructor for object literal
    static ParsedExpressionPtr makeObject(std::vector<std::pair<std::string, ParsedExpressionPtr>> members, const std::string &filename, int line, size_t column) {
        auto expr = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Object;
        expr->objectMembers = std::move(members);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    static ParsedExpressionPtr makeMember(ParsedExpressionPtr object, const std::string &propName, const std::string &filename, int line, size_t column) {
        auto expr = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Member;
        expr->objectMembers.push_back({propName, std::move(object)});
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    Symbols::Variables::Type getType() const;
    std::string toString() const;
};

}  // namespace Parser

#endif  // PARSEDEXPRESSION_HPP
