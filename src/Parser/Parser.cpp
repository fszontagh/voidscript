#include "Parser/Parser.hpp"
#include <stack>

#include "Interpreter/OperationsFactory.hpp"
#include "Lexer/Operators.hpp"

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
    // ... other keywords ...
};

const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> Parser::variable_types = {
    { Lexer::Tokens::Type::KEYWORD_INT,     Symbols::Variables::Type::INTEGER   },
    { Lexer::Tokens::Type::KEYWORD_DOUBLE,  Symbols::Variables::Type::DOUBLE    },
    { Lexer::Tokens::Type::KEYWORD_FLOAT,   Symbols::Variables::Type::FLOAT     },
    { Lexer::Tokens::Type::KEYWORD_STRING,  Symbols::Variables::Type::STRING    },
    { Lexer::Tokens::Type::KEYWORD_NULL,    Symbols::Variables::Type::NULL_TYPE },
    { Lexer::Tokens::Type::KEYWORD_BOOLEAN, Symbols::Variables::Type::BOOLEAN   },
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
    auto len = closing_brace.start_pos - opening_brace.end_pos;
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

        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "(") {
            operator_stack.push("(");
            consumeToken();
            expect_unary = true;
        } else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == ")") {
            consumeToken();
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
                reportError("Mismatched parentheses");
            }
            operator_stack.pop();  // remove "("
            expect_unary = false;
        } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC) {
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
                            reportError("Missing operand for unary operator");
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(Lexer::applyOperator(top, std::move(rhs)));
                    } else {
                        if (output_queue.size() < 2) {
                            reportError("Malformed expression");
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
        } else if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                   token.type == Lexer::Tokens::Type::KEYWORD ||
                   token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
            if (Lexer::pushOperand(token, expected_var_type, output_queue) == false) {
                reportError("Expected literal or variable");
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
            reportError("Mismatched parentheses");
        }

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
