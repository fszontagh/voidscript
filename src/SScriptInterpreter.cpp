#include "SScriptInterpreter.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "Lexer.hpp"
#include "ScriptExceptionMacros.h"
#include "Value.hpp"

void SScriptInterpreter::registerFunction(const std::string & name, std::shared_ptr<BaseFunction> fn) {
    functionObjects[name] = std::move(fn);
}

Value SScriptInterpreter::evaluateExpression(const Token & token) const {
    if (token.type == TokenType::StringLiteral) {
        return Value::fromString(token.lexeme);
    }
    if (token.type == TokenType::IntLiteral) {
        try {
            return Value::fromInt(std::stoi(token.lexeme));
        } catch (const std::invalid_argument & e) {
            throw std::runtime_error("Invalid integer literal: " + token.lexeme);
        } catch (const std::out_of_range & e) {
            throw std::runtime_error("Integer literal out of range: " + token.lexeme);
        }
    }
    if (token.type == TokenType::DoubleLiteral) {
        try {
            return Value::fromDouble(std::stod(token.lexeme));
        } catch (const std::invalid_argument & e) {
            throw std::runtime_error("Invalid double literal: " + token.lexeme);
        } catch (const std::out_of_range & e) {
            throw std::runtime_error("Double literal out of range: " + token.lexeme);
        }
    }
    if (token.type == TokenType::Variable) {
        if (variables.find(token.lexeme) != variables.end()) {
            return variables.at(token.lexeme);
        }
        THROW_UNDEFINED_VARIABLE_ERROR(token.lexeme, token);
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(token, "string, integer, double, or variable");
    }
    return Value();
}

std::vector<Value> SScriptInterpreter::parseArguments(const std::vector<Token> & tokens,
                                                      std::size_t &              current_index) const {
    std::vector<Value> args;

    if (current_index >= tokens.size() || tokens[current_index].type != TokenType::Identifier) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], tokenTypeNames.at(TokenType::Identifier));
    }
    current_index++;  // Skip function name

    if (current_index >= tokens.size() || tokens[current_index].type != TokenType::LeftParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], tokenTypeNames.at(TokenType::LeftParenthesis));
    }
    current_index++;  // Skip '('

    while (current_index < tokens.size() && tokens[current_index].type != TokenType::RightParenthesis) {
        args.push_back(evaluateExpression(tokens[current_index]));
        current_index++;
        if (current_index < tokens.size() && tokens[current_index].type == TokenType::Comma) {
            current_index++;  // Skip ','
            if (current_index >= tokens.size() || tokens[current_index].type == TokenType::RightParenthesis) {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], "expression after comma");
            }
        } else if (tokens[current_index].type != TokenType::RightParenthesis && current_index < tokens.size()) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], "',' or ')'");
        }
    }

    if (current_index >= tokens.size() || tokens[current_index].type != TokenType::RightParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], "')'");
    }
    current_index++;  // Skip ')'
    current_index = current_index;

    return args;
}

void SScriptInterpreter::handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i) {
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='

        if (i < tokens.size() && tokens[i].type == TokenType::Variable) {
            if (variables.find(tokens[i].lexeme) == variables.end()) {
                THROW_UNDEFINED_VARIABLE_ERROR(tokens[i].lexeme, tokens[i]);
            } else {
                if (variables[tokens[i].lexeme].type != Variables::Type::VT_STRING) {
                    THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName, Variables::TypeToString(Variables::Type::VT_STRING),
                                                        tokens[i].lexeme, variables[tokens[i].lexeme].TypeToString(),
                                                        tokens[i]);
                }

                variables[varName] = variables[tokens[i].lexeme];
                i++;  // Skip variable name
                expectSemicolon(tokens, i, "after string variable declaration");
            }
        } else if (i < tokens.size() && tokens[i].type == TokenType::StringLiteral) {
            variables[varName] = Value::fromString(tokens[i].lexeme);
            i++;  // Skip string literal
            expectSemicolon(tokens, i, "after string declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "string literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after string declaration");
    }
}

void SScriptInterpreter::handleNumberDeclaration(const std::vector<Token> & tokens, std::size_t & i, TokenType type) {
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            if (type == TokenType::IntDeclaration && tokens[i].type == TokenType::IntLiteral) {
                try {
                    if (variables.find(varName) != variables.end()) {
                        THROW_VARIABLE_REDEFINITION_ERROR(varName, tokens[i]);
                    }
                    variables[varName] = Value::fromInt(std::stoi(tokens[i].lexeme));
                    i++;  // Skip int literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid integer literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Integer literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else if (type == TokenType::DoubleDeclaration && tokens[i].type == TokenType::DoubleLiteral) {
                try {
                    if (variables.find(varName) != variables.end()) {
                        THROW_VARIABLE_REDEFINITION_ERROR(varName, tokens[i]);
                    }
                    variables[varName] = Value::fromDouble(std::stod(tokens[i].lexeme));
                    i++;  // Skip double literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid double literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Double literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else {
                const std::string expectedType = type == TokenType::IntDeclaration ? "int" : "double";
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName, expectedType, "",
                                                    getVariableTypeFromTokenTypeAsString(tokens[i].type), tokens[i]);
            }
            expectSemicolon(tokens, i, "after variable declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after variable declaration, variable name: " + varName);
    }
}

void SScriptInterpreter::handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i) {
    std::string funcName = tokens[i].lexeme;
    auto        it       = functionObjects.find(funcName);
    if (it == functionObjects.end()) {
        throw std::runtime_error("Unknown function: " + funcName);
    }
    it->second->validate(tokens, i);
    std::vector<Value> args = parseArguments(tokens, i);
    it->second->call(args);
    if (i < tokens.size() && tokens[i].type == TokenType::Semicolon) {
        i++;  // Skip ';' after function call
    }
}

void SScriptInterpreter::handleVariableReference(const std::vector<Token> & tokens, std::size_t & i) {
    //THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "function call or variable assignment (not yet implemented)");
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;
    i++;      // Skip variable token to avoid infinite loop
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            if (variables.find(varName) == variables.end()) {
                THROW_UNDEFINED_VARIABLE_ERROR(varName, tokens[i]);
            }
            variables[varName] = evaluateExpression(tokens[i]);
            i++;  // Skip value
            expectSemicolon(tokens, i, "after variable assignment");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "value after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "'=' for assignment");
    }
}

void SScriptInterpreter::handleComment(std::size_t & i) {
    i++;  // Skip comment token
}

void SScriptInterpreter::handleSemicolon(std::size_t & i) {
    i++;  // Skip semicolon token
}

void SScriptInterpreter::expectSemicolon(const std::vector<Token> & tokens, std::size_t & i,
                                         const std::string & message) const {
    if (i >= tokens.size() || tokens[i].type != TokenType::Semicolon) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "; " + message);
    } else {
        i++;  // Skip ';'
    }
}

void SScriptInterpreter::executeScript(const std::string & source, const std::string & filename, bool debug) {
    Lexer lexer(source, filename);
    auto  tokens = lexer.tokenize();

    bool insideScript = false;

    for (std::size_t i = 0; i < tokens.size();) {
        const auto & token = tokens[i];

        if (token.type == TokenType::EndOfFile) {
            break;
        }

        if (token.type == TokenType::ParserOpenTag) {
            insideScript = true;
            i++;  // Skip the open tag
            continue;
        }

        if (token.type == TokenType::ParserCloseTag) {
            insideScript = false;
            i++;  // Skip the close tag
            continue;
        }

        if (!insideScript) {
            // Csak kiíratás, ha nem vagyunk script tagben
            std::cout << token.lexeme;
            i++;
            continue;
        }

        // A szokásos feldolgozás csak ha belül vagyunk
        switch (token.type) {
            case TokenType::StringDeclaration:
                handleStringDeclaration(tokens, i);
                break;
            case TokenType::IntDeclaration:
            case TokenType::DoubleDeclaration:
                handleNumberDeclaration(tokens, i, token.type);
                break;
            case TokenType::Identifier:
                handleFunctionCall(tokens, i);
                break;
            case TokenType::Variable:
                handleVariableReference(tokens, i);
                break;
            case TokenType::Comment:
                handleComment(i);
                break;
            case TokenType::Semicolon:
                handleSemicolon(i);
                break;
            default:
                throw std::runtime_error("Unexpected token inside script: " + token.lexeme);
        }
    }
}
