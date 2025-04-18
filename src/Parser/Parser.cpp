#include "Parser/Parser.hpp"

#include <stack>

#include "Interpreter/OperationsFactory.hpp"
#include "Lexer/Operators.hpp"
// Statements and expression building for conditional and block parsing
#include "Interpreter/ConditionalStatementNode.hpp"
// #include "Interpreter/ForStatementNode.hpp"  // removed until for-in loops are implemented
#include "Interpreter/CallStatementNode.hpp"
#include "Interpreter/AssignmentStatementNode.hpp"
#include "Interpreter/DeclareVariableStatementNode.hpp"
#include "Interpreter/ReturnStatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Interpreter/ExpressionBuilder.hpp"

// Additional necessary includes, if needed
namespace Parser {

const std::unordered_map<std::string, Lexer::Tokens::Type> Parser::keywords = {
    { "if",       Lexer::Tokens::Type::KEYWORD                      },
    { "else",     Lexer::Tokens::Type::KEYWORD                      },
    { "while",    Lexer::Tokens::Type::KEYWORD                      },
    { "for",      Lexer::Tokens::Type::KEYWORD                      },
    { "return",   Lexer::Tokens::Type::KEYWORD_RETURN               },
    { "function", Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION },
    // Older keywords:
    { "const",    Lexer::Tokens::Type::KEYWORD                      },
    { "true",     Lexer::Tokens::Type::KEYWORD                      },
    { "false",    Lexer::Tokens::Type::KEYWORD                      },
    // variable types
    { "null",     Lexer::Tokens::Type::KEYWORD_NULL                 },
    { "int",      Lexer::Tokens::Type::KEYWORD_INT                  },
    { "double",   Lexer::Tokens::Type::KEYWORD_DOUBLE               },
    { "float",    Lexer::Tokens::Type::KEYWORD_FLOAT                },
    { "string",   Lexer::Tokens::Type::KEYWORD_STRING               },
    { "boolean",  Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "bool",     Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "object",   Lexer::Tokens::Type::KEYWORD_OBJECT               },
    // ... other keywords ...
};

const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> Parser::variable_types = {
    { Lexer::Tokens::Type::KEYWORD_INT,     Symbols::Variables::Type::INTEGER   },
    { Lexer::Tokens::Type::KEYWORD_DOUBLE,  Symbols::Variables::Type::DOUBLE    },
    { Lexer::Tokens::Type::KEYWORD_FLOAT,   Symbols::Variables::Type::FLOAT     },
    { Lexer::Tokens::Type::KEYWORD_STRING,  Symbols::Variables::Type::STRING    },
    { Lexer::Tokens::Type::KEYWORD_NULL,    Symbols::Variables::Type::NULL_TYPE },
    { Lexer::Tokens::Type::KEYWORD_BOOLEAN, Symbols::Variables::Type::BOOLEAN   },
    { Lexer::Tokens::Type::KEYWORD_OBJECT,  Symbols::Variables::Type::OBJECT    },
};

void Parser::parseVariableDefinition() {
    Symbols::Variables::Type var_type = parseType();

    Lexer::Tokens::Token id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
    std::string          var_name = id_token.value;

    if (!var_name.empty() && var_name[0] == '$') {
        var_name = var_name.substr(1);
    }
    const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();

    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");

    auto expr = parseParsedExpression(var_type);
    Interpreter::OperationsFactory::defineVariableWithExpression(
        var_name, var_type, std::move(expr), ns, current_filename_, id_token.line_number, id_token.column_number);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
}

// Parse a top-level assignment statement and record it
void Parser::parseAssignmentStatement() {
    auto stmt = parseStatementNode();
    Operations::Container::instance()->add(
        Symbols::SymbolContainer::instance()->currentScopeName(),
        Operations::Operation{Operations::Type::Assignment, "", std::move(stmt)}
    );
}

// Parse an if-else conditional statement
void Parser::parseIfStatement() {
    // 'if'
    auto ifToken = expect(Lexer::Tokens::Type::KEYWORD, "if");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse the condition expression without restricting literal types,
    // dynamic evaluation will enforce boolean type at runtime
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    // then branch
    std::vector<std::unique_ptr<Interpreter::StatementNode>> thenBranch;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        thenBranch.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    // else branch
    std::vector<std::unique_ptr<Interpreter::StatementNode>> elseBranch;
    if (match(Lexer::Tokens::Type::KEYWORD, "else")) {
        expect(Lexer::Tokens::Type::PUNCTUATION, "{");
        while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
            elseBranch.push_back(parseStatementNode());
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    }
    // build condition node
    auto condNode = buildExpressionFromParsed(condExpr);
    auto stmt = std::make_unique<Interpreter::ConditionalStatementNode>(
        std::move(condNode), std::move(thenBranch), std::move(elseBranch),
        this->current_filename_, ifToken.line_number, ifToken.column_number);
    // add conditional operation
    Operations::Container::instance()->add(
        Symbols::SymbolContainer::instance()->currentScopeName(),
        Operations::Operation{Operations::Type::Conditional, "", std::move(stmt)});
}

// Parse a single statement and return its StatementNode (for use in blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseStatementNode() {
    // Return statement
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        auto tok = expect(Lexer::Tokens::Type::KEYWORD_RETURN);
        ParsedExpressionPtr expr = nullptr;
        if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ";")) {
            expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = expr ? buildExpressionFromParsed(expr) : nullptr;
        return std::make_unique<Interpreter::ReturnStatementNode>(
            std::move(exprNode), this->current_filename_, tok.line_number, tok.column_number);
    }
    // Assignment statement: variable or object member assignment
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        // Lookahead to detect '=' after optional '->' chains
        size_t offset = 1;
        // Skip member access sequence
        while (peekToken(offset).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(offset).value == "->") {
            offset += 2;  // skip '->' and following identifier
        }
        const auto & look = peekToken(offset);
        if (look.type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT && look.value == "=") {
            // Consume base variable
            auto idTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string baseName = idTok.value;
            if (!baseName.empty() && baseName[0] == '$') baseName = baseName.substr(1);
            // Collect member path keys
            std::vector<std::string> propertyPath;
            while (match(Lexer::Tokens::Type::PUNCTUATION, "->")) {
                // Next token must be identifier or variable identifier
                Lexer::Tokens::Token propTok;
                if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                    currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                    propTok = consumeToken();
                } else {
                    reportError("Expected property name after '->'");
                }
                std::string propName = propTok.value;
                if (!propName.empty() && propName[0] == '$') propName = propName.substr(1);
                propertyPath.push_back(propName);
            }
            // Consume '='
            auto eqTok = expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
            // Parse RHS expression
            auto rhsExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            // Build RHS node
            auto rhsNode = buildExpressionFromParsed(rhsExpr);
            return std::make_unique<Interpreter::AssignmentStatementNode>(
                baseName, std::move(propertyPath), std::move(rhsNode),
                this->current_filename_, eqTok.line_number, eqTok.column_number);
        }
    }
    // Function call statement
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        auto idTok = expect(Lexer::Tokens::Type::IDENTIFIER);
        std::string funcName = idTok.value;
        expect(Lexer::Tokens::Type::PUNCTUATION, "(");
        std::vector<ParsedExpressionPtr> args;
        if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
            while (true) {
                args.push_back(parseParsedExpression(Symbols::Variables::Type::NULL_TYPE));
                if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) continue;
                break;
            }
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        std::vector<std::unique_ptr<Interpreter::ExpressionNode>> exprs;
        exprs.reserve(args.size());
        for (auto &p : args) {
            exprs.push_back(buildExpressionFromParsed(p));
        }
        return std::make_unique<Interpreter::CallStatementNode>(
            funcName, std::move(exprs), this->current_filename_, idTok.line_number, idTok.column_number);
    }
    // Variable declaration
    if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end()) {
        auto type = parseType();
        auto idTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string name = idTok.value;
        if (!name.empty() && name[0] == '$') name = name.substr(1);
        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        auto valExpr = parseParsedExpression(type);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = buildExpressionFromParsed(valExpr);
        return std::make_unique<Interpreter::DeclareVariableStatementNode>(
            name, Symbols::SymbolContainer::instance()->currentScopeName(), type,
            std::move(exprNode), this->current_filename_, idTok.line_number, idTok.column_number);
    }
    reportError("Unexpected token in block");
    return nullptr;
}

void Parser::parseFunctionDefinition() {
    expect(Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION);
    Lexer::Tokens::Token     id_token         = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string              func_name        = id_token.value;
    Symbols::Variables::Type func_return_type = Symbols::Variables::Type::NULL_TYPE;
    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");

    Symbols::FunctionParameterInfo param_infos;

    if (currentToken().type != Lexer::Tokens::Type::PUNCTUATION || currentToken().value != ")") {
        while (true) {
            // Parameter type
            Symbols::Variables::Type param_type = parseType();  // This consumes the type token

            // Parameter name ($variable)
            Lexer::Tokens::Token param_id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string          param_name     = param_id_token.value;
            if (!param_name.empty() && param_name[0] == '$') {  // remove '$'
                param_name = param_name.substr(1);
            }

            param_infos.push_back({ param_name, param_type });

            // Expecting comma or closing parenthesis?
            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                continue;
            }
            if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")") {
                break;  // end of list
            }
            reportError("Expected ',' or ')' in parameter list");
        }
    }
    // Now expect ')'
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    // check if we have a option return type: function name() type { ... }
    for (const auto & _type : Parser::variable_types) {
        if (match(_type.first)) {
            func_return_type = _type.second;
            break;
        }
    }

    Lexer::Tokens::Token opening_brace = expect(Lexer::Tokens::Type::PUNCTUATION, "{");

    // only parse the body if we checked out if not exists the function and created the symbol
    parseFunctionBody(opening_brace, func_name, func_return_type, param_infos);
}

// Parse a top-level function call, e.g., foo(arg1, arg2);
void Parser::parseCallStatement() {
    // Function name
    auto        id_token  = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string func_name = id_token.value;
    // Opening parenthesis
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse comma-separated argument expressions
    std::vector<ParsedExpressionPtr> args;
    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
        while (true) {
            // Parse expression with no expected type
            auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            args.push_back(std::move(expr));
            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                continue;
            }
            break;
        }
    }
    // Closing parenthesis and semicolon
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
    // Record the function call operation
    Interpreter::OperationsFactory::callFunction(
        func_name,
        std::move(args),
        Symbols::SymbolContainer::instance()->currentScopeName(),
        this->current_filename_,
        id_token.line_number,
        id_token.column_number);
}

// Parse a return statement, e.g., return; or return expression;
void Parser::parseReturnStatement() {
    // Consume 'return' keyword
    auto returnToken = expect(Lexer::Tokens::Type::KEYWORD_RETURN);
    // Parse optional expression
    ParsedExpressionPtr expr = nullptr;
    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ";")) {
        expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    }
    // Record return operation
    Interpreter::OperationsFactory::callReturn(
        expr,
        Symbols::SymbolContainer::instance()->currentScopeName(),
        this->current_filename_,
        returnToken.line_number,
        returnToken.column_number);
    // Consume terminating semicolon
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
}

// Continue with numeric literal parsing
//
Symbols::Value Parser::parseNumericLiteral(const std::string & value, bool is_negative, Symbols::Variables::Type type) {
    try {
        switch (type) {
            case Symbols::Variables::Type::INTEGER:
                {
                    if (value.find('.') != std::string::npos) {
                        throw std::invalid_argument("Floating point value in integer context: " + value);
                    }
                    int v = std::stoi(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::DOUBLE:
                {
                    double v = std::stod(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::FLOAT:
                {
                    float v = std::stof(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            default:
                throw std::invalid_argument("Unsupported numeric type");
        }
    } catch (const std::invalid_argument & e) {
        reportError("Invalid numeric literal: " + value + " (" + e.what() + ")");
    } catch (const std::out_of_range & e) {
        reportError("Numeric literal out of range: " + value + " (" + e.what() + ")");
    }

    return Symbols::Value();  // unreachable
}

void Parser::parseFunctionBody(const Lexer::Tokens::Token & opening_brace, const std::string & function_name,
                               Symbols::Variables::Type return_type, const Symbols::FunctionParameterInfo & params) {
    size_t               braceDepth = 0;
    int                  peek       = 0;
    int                  tokenIndex = current_token_index_;
    Lexer::Tokens::Token currentToken_;
    Lexer::Tokens::Token closing_brace;

    while (tokenIndex < tokens_.size()) {
        currentToken_ = peekToken(peek);
        if (currentToken_.type == Lexer::Tokens::Type::PUNCTUATION) {
            if (currentToken_.value == "{") {
                ++braceDepth;
            } else if (currentToken_.value == "}") {
                if (braceDepth == 0) {
                    closing_brace = currentToken_;
                    break;
                }
                --braceDepth;
            }
        }
        tokenIndex++;
        peek++;
    }
    if (braceDepth != 0) {
        reportError("Unmatched braces in function body");
    }
    std::vector<Lexer::Tokens::Token> filtered_tokens;
    auto                              startIt = std::find(tokens_.begin(), tokens_.end(), opening_brace);
    auto                              endIt   = std::find(tokens_.begin(), tokens_.end(), closing_brace);

    if (startIt != tokens_.end() && endIt != tokens_.end() && startIt < endIt) {
        filtered_tokens = std::vector<Lexer::Tokens::Token>(startIt + 1, endIt);
    }
    auto             len          = closing_brace.start_pos - opening_brace.end_pos;
    std::string_view input_string = input_str_view_.substr(opening_brace.end_pos, len);

    current_token_index_ = tokenIndex;
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    const std::string newns = Symbols::SymbolContainer::instance()->currentScopeName() + "." + function_name;
    Symbols::SymbolContainer::instance()->create(newns);
    // Parse function body using a stack‑allocated Parser (avoid heap allocations)
    Parser innerParser;
    innerParser.parseScript(filtered_tokens, input_string, this->current_filename_);
    Symbols::SymbolContainer::instance()->enterPreviousScope();
    // create function
    Interpreter::OperationsFactory::defineFunction(
        function_name, params, return_type, Symbols::SymbolContainer::instance()->currentScopeName(),
        this->current_filename_, currentToken_.line_number, currentToken_.column_number);
}

ParsedExpressionPtr Parser::parseParsedExpression(const Symbols::Variables::Type & expected_var_type) {
    std::stack<std::string>          operator_stack;
    std::vector<ParsedExpressionPtr> output_queue;
    // Reserve output queue to reduce reallocations
    if (tokens_.size() > current_token_index_) {
        output_queue.reserve(tokens_.size() - current_token_index_);
    }

    bool expect_unary = true;

    while (true) {
        auto token = currentToken();
        // Object literal: { key: value, ... }
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "{") {
            // Consume '{'
            consumeToken();
            std::vector<std::pair<std::string, ParsedExpressionPtr>> members;
            // Parse members until '}'
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
                while (true) {
                    // Optional type tag before key
                    Symbols::Variables::Type memberType = Symbols::Variables::Type::UNDEFINED_TYPE;
                    if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end()) {
                        memberType = parseType();
                    }
                    // Key must be an identifier or variable identifier
                    if (currentToken().type != Lexer::Tokens::Type::IDENTIFIER &&
                        currentToken().type != Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                        reportError("Expected identifier for object key");
                    }
                    std::string key = currentToken().value;
                    // Strip '$' if present
                    if (!key.empty() && key[0] == '$') {
                        key = key.substr(1);
                    }
                    consumeToken();
                    // Expect ':' delimiter
                    expect(Lexer::Tokens::Type::PUNCTUATION, ":");
                    // Parse value expression (pass tag type if provided)
                    Symbols::Variables::Type expectType = (memberType == Symbols::Variables::Type::UNDEFINED_TYPE)
                                                            ? Symbols::Variables::Type::NULL_TYPE
                                                            : memberType;
                    auto valueExpr = parseParsedExpression(expectType);
                    members.emplace_back(key, std::move(valueExpr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            // Expect closing '}'
            expect(Lexer::Tokens::Type::PUNCTUATION, "}");
            // Create object literal parsed expression
            output_queue.push_back(ParsedExpression::makeObject(std::move(members)));
            expect_unary = false;
            continue;
        }

        // Member access: '->'
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "->") {
            std::string op(token.lexeme);
            // Shunting-yard: handle operator precedence
            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                if ((Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) <= Lexer::getPrecedence(top)) ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) < Lexer::getPrecedence(top))) {
                    operator_stack.pop();
                    // Binary operator: pop two operands
                    if (output_queue.size() < 2) {
                        Parser::reportError("Malformed expression", token);
                    }
                    auto rhs = std::move(output_queue.back()); output_queue.pop_back();
                    auto lhs = std::move(output_queue.back()); output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
                } else {
                    break;
                }
            }
            operator_stack.push(op);
            consumeToken();
            expect_unary = true;
        }
        // Grouping parentheses
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "(") {
            operator_stack.push("(");
            consumeToken();
            expect_unary = true;
        } else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == ")") {
            // Only handle grouping parentheses if a matching "(" exists on the operator stack
            std::stack<std::string> temp_stack = operator_stack;
            bool has_paren = false;
            while (!temp_stack.empty()) {
                if (temp_stack.top() == "(") {
                    has_paren = true;
                    break;
                }
                temp_stack.pop();
            }
            if (!has_paren) {
                // End of this expression context; do not consume call-closing parenthesis here
                break;
            }
            // Consume the grouping closing parenthesis
            consumeToken();
            // Unwind operators until the matching "(" is found
            while (!operator_stack.empty() && operator_stack.top() != "(") {
                std::string op = operator_stack.top();
                operator_stack.pop();

                if (op == "u-" || op == "u+") {
                    if (output_queue.empty()) {
                        reportError("Missing operand for unary operator");
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs)));
                } else {
                    if (output_queue.size() < 2) {
                        reportError("Malformed expression");
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    auto lhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
                }
            }
            if (operator_stack.empty() || operator_stack.top() != "(") {
                Parser::reportError("Mismatched parentheses", token);
            }
            // Pop the matching "("
            operator_stack.pop();
            expect_unary = false;
        }
        // Function call as expression: identifier followed by '('
        else if (token.type == Lexer::Tokens::Type::IDENTIFIER &&
                 peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
            // Parse function call
            std::string func_name = token.value;
            consumeToken();  // consume function name
            consumeToken();  // consume '('
            std::vector<ParsedExpressionPtr> call_args;
            // Parse arguments if any
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                while (true) {
                    auto arg_expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                    call_args.push_back(std::move(arg_expr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            // Create call expression node
            output_queue.push_back(ParsedExpression::makeCall(func_name, std::move(call_args)));
            expect_unary = false;
        } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC
                   || token.type == Lexer::Tokens::Type::OPERATOR_RELATIONAL
                   || token.type == Lexer::Tokens::Type::OPERATOR_LOGICAL) {
            std::string op = std::string(token.lexeme);

            if (expect_unary && Lexer::isUnaryOperator(op)) {
                op = "u" + op;  // e.g. u-, u+ or u!
            }

            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                if ((Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) <= Lexer::getPrecedence(top)) ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) < Lexer::getPrecedence(top))) {
                    operator_stack.pop();

                    if (top == "u-" || top == "u+") {
                        if (output_queue.empty()) {
                            Parser::reportError("Missing operand for unary operator", token);
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(Lexer::applyOperator(top, std::move(rhs)));
                    } else {
                        if (output_queue.size() < 2) {
                            Parser::reportError("Malformed expression", token);
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        auto lhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(Lexer::applyOperator(top, std::move(rhs), std::move(lhs)));
                    }
                } else {
                    break;
                }
            }

            operator_stack.push(op);
            consumeToken();
            expect_unary = true;
        } else if (token.type == Lexer::Tokens::Type::NUMBER
                   || token.type == Lexer::Tokens::Type::STRING_LITERAL
                   || token.type == Lexer::Tokens::Type::KEYWORD
                   || token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER
                   || token.type == Lexer::Tokens::Type::IDENTIFIER) {
            if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
                // Treat bare identifiers as variable references for member access
                output_queue.push_back(ParsedExpression::makeVariable(token.value));
            } else {
                if (Lexer::pushOperand(token, expected_var_type, output_queue) == false) {
                    Parser::reportError("Invalid type", token, "literal or variable");
                }
            }
            consumeToken();
            expect_unary = false;
        } else {
            break;
        }
    }

    // Empty the operator stack
    while (!operator_stack.empty()) {
        std::string op = operator_stack.top();
        operator_stack.pop();

        if (op == "(" || op == ")") {
            Parser::reportError("Mismatched parentheses", tokens_[current_token_index_]);
        }

        // Handle unary operators (plus, minus, logical NOT)
        if (op == "u-" || op == "u+" || op == "u!") {
            if (output_queue.empty()) {
                Parser::reportError("Missing operand for unary operator", tokens_[current_token_index_]);
            }
            auto rhs = std::move(output_queue.back());
            output_queue.pop_back();
            output_queue.push_back(Lexer::applyOperator(op, std::move(rhs)));
        } else {
            // Binary operators
            if (output_queue.size() < 2) {
                Parser::reportError("Malformed expression", tokens_[current_token_index_]);
            }
            auto rhs = std::move(output_queue.back());
            output_queue.pop_back();
            auto lhs = std::move(output_queue.back());
            output_queue.pop_back();
            output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
        }
    }

    if (output_queue.size() != 1) {
        reportError("Expression could not be parsed cleanly");
    }

    return std::move(output_queue.back());
}

void Parser::parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                         const std::string & filename) {
    tokens_              = tokens;
    input_str_view_      = input_string;
    current_token_index_ = 0;
    current_filename_    = filename;

    while (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        parseStatement();
    }
    if (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        reportError("Unexpected tokens after program end");
    }
}
}  // namespace Parser
