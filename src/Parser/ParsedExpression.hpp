#ifndef PARSEDEXPRESSION_HPP
#define PARSEDEXPRESSION_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../Symbols/FunctionSymbol.hpp"
#include "../Symbols/SymbolContainer.hpp"
#include "../Symbols/Value.hpp"

namespace Parser {

struct ParsedExpression;

using ParsedExpressionPtr = std::shared_ptr<ParsedExpression>;

struct ParsedExpression {
    enum class Kind : std::uint8_t { Literal, Variable, Binary, Unary, Call, MethodCall, New, Object, Member, Unknown };

    static std::string kindToString(ParsedExpression::Kind kind) {
        const std::unordered_map<Kind, std::string> kindstringmap = {
            { Kind::Literal,    "Literal"    },
            { Kind::Variable,   "Variable"   },
            { Kind::Binary,     "Binary"     },
            { Kind::Unary,      "Unary"      },
            { Kind::Call,       "Call"       },
            { Kind::MethodCall, "MethodCall" },
            { Kind::New,        "New"        },
            { Kind::Object,     "Object"     },
            { Kind::Member,     "Member"     },
            { Kind::Unknown,    "Unknown"    }
        };

        return kindstringmap.at(kind);
    }

    Kind kind;

    Symbols::ValuePtr value;
    std::string       name;

    // For operations
    std::string                                              op;
    ParsedExpressionPtr                                      lhs;
    ParsedExpressionPtr                                      rhs;
    // For function call arguments
    std::vector<ParsedExpressionPtr>                         args;
    std::vector<std::pair<std::string, ParsedExpressionPtr>> objectMembers;
    // Source location for error reporting
    std::string                                              filename;
    int                                                      line   = 0;
    size_t                                                   column = 0;

    // Constructor for literal
    static ParsedExpressionPtr makeLiteral(Symbols::ValuePtr val) {
        auto expr   = std::make_shared<ParsedExpression>();
        expr->kind  = Kind::Literal;
        expr->value = std::move(val);
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
    static ParsedExpressionPtr makeBinary(std::string op, ParsedExpressionPtr left, ParsedExpressionPtr right,
                                          const std::string & filename, int line, size_t column) {
        auto expr      = std::make_shared<ParsedExpression>();
        expr->kind     = Kind::Binary;
        expr->op       = std::move(op);
        expr->lhs      = std::move(left);
        expr->rhs      = std::move(right);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    // Constructor for unary operation
    static ParsedExpressionPtr makeUnary(std::string op, ParsedExpressionPtr operand, const std::string & filename,
                                         int line, size_t column) {
        auto expr      = std::make_shared<ParsedExpression>();
        expr->kind     = Kind::Unary;
        expr->op       = std::move(op);
        expr->rhs      = std::move(operand);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    // Constructor for function call
    static ParsedExpressionPtr makeCall(const std::string & name, std::vector<ParsedExpressionPtr> arguments,
                                        const std::string & filename, int line, size_t column) {
        auto expr      = std::make_shared<ParsedExpression>();
        expr->kind     = Kind::Call;
        expr->name     = name;
        expr->args     = std::move(arguments);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    // Constructor for method call: object->method(args)
    static ParsedExpressionPtr makeMethodCall(ParsedExpressionPtr object, const std::string & methodName,
                                              std::vector<ParsedExpressionPtr> arguments, const std::string & filename,
                                              int line, size_t column) {
        auto expr      = std::make_shared<ParsedExpression>();
        expr->kind     = Kind::MethodCall;
        expr->lhs      = std::move(object);
        expr->name     = methodName;
        expr->args     = std::move(arguments);
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    // Constructor for 'new' expression: instantiate class
    static ParsedExpressionPtr makeNew(const std::string & className, std::vector<ParsedExpressionPtr> arguments,
                                       const std::string & filename, int line, size_t column) {
        // Create the new expression node
        auto expr = std::make_shared<ParsedExpression>();
        expr->kind = Kind::New;
        expr->name = className;  // Store the class name
        expr->args = std::move(arguments);  // Store constructor arguments
        expr->filename = filename;
        expr->line = line;
        expr->column = column;
        
        return expr;
    }

    // Constructor for object literal
    static ParsedExpressionPtr makeObject(std::vector<std::pair<std::string, ParsedExpressionPtr>> members,
                                          const std::string & filename, int line, size_t column) {
        auto expr           = std::make_shared<ParsedExpression>();
        expr->kind          = Kind::Object;
        expr->objectMembers = std::move(members);
        expr->filename      = filename;
        expr->line          = line;
        expr->column        = column;

        return expr;
    }

    static ParsedExpressionPtr makeMember(ParsedExpressionPtr object, const std::string & propName,
                                          const std::string & filename, int line, size_t column) {
        auto expr  = std::make_shared<ParsedExpression>();
        expr->kind = Kind::Member;
        expr->objectMembers.push_back({ propName, std::move(object) });
        expr->filename = filename;
        expr->line     = line;
        expr->column   = column;

        return expr;
    }

    Symbols::Variables::Type getType() const {
        switch (kind) {
            case Kind::Literal:
                return value.getType();
                break;

            case Kind::Variable:
                {
                    // First try to get the variable
                    auto symbol = Symbols::SymbolContainer::instance()->getVariable(name);
                    
                    // If not found as a variable, try as a constant
                    if (!symbol) {
                        symbol = Symbols::SymbolContainer::instance()->getConstant(name);
                    }
                    
                    if (!symbol) {
                        throw std::runtime_error("Unknown variable or constant: " + name + " (searched from scope: " +
                                                 Symbols::SymbolContainer::instance()->currentScopeName() + ")" +
                                                 " File: " + filename + ":" + std::to_string(line));
                    }
                    
                    return symbol->getValue().getType();
                }

            case Kind::Binary:
                {
                    return lhs->value.getType();
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
                    // For getType during parsing, we typically check the current scope for the function.
                    // A more advanced type system might search up the scope chain.
                    auto *             sc                  = Symbols::SymbolContainer::instance();
                    auto               current_scope_table = sc->getScopeTable(sc->currentScopeName());
                    Symbols::SymbolPtr symbol              = nullptr;
                    if (current_scope_table) {
                        symbol = current_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, name);
                    }

                    if (!symbol) {
                        throw std::runtime_error("Unknown function: " + name +
                                                 " in current scope: " + sc->currentScopeName() + " File: " + filename +
                                                 ":" + std::to_string(line));
                    }
                    // FunctionSymbol holds return type
                    auto funcSym = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(symbol);
                    if (!funcSym) {
                        // Should not happen if it was found in DEFAULT_FUNCTIONS_SCOPE and is a function
                        throw std::runtime_error("Symbol " + name + " found but is not a function." +
                                                 " File: " + filename + ":" + std::to_string(line));
                    }
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
                return value.toString();
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
                        if (i > 0) {
                            result += ", ";
                        }
                        result += args[i]->toString();
                    }
                    result += ")";
                    return result;
                }
            case Kind::MethodCall:
                {
                    std::string result;
                    if (lhs && lhs->kind == Kind::Variable) {
                        result = lhs->name;
                    } else if (lhs) {
                        result = lhs->toString();
                    }
                    result += "->" + name + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) {
                            result += ", ";
                        }
                        result += args[i]->toString();
                    }
                    result += ")";
                    return result;
                }
            case Kind::Object:
                {
                    std::string result = "{";
                    for (size_t i = 0; i < objectMembers.size(); ++i) {
                        if (i > 0) {
                            result += ", ";
                        }
                        result += objectMembers[i].first + ": " + objectMembers[i].second->toString();
                    }
                    result += "}";
                    return result;
                }
            case Kind::New:
                {
                    std::string result = "new " + name + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) {
                            result += ", ";
                        }
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
