#include "ScriptInterpreter.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "Lexer.hpp"
#include "ScriptExceptionMacros.h"
#include "Value.hpp"

void ScriptInterpreter::registerModule(const std::string & name, std::shared_ptr<BaseFunction> fn) {
    functionObjects[name] = std::move(fn);
}

Value ScriptInterpreter::evaluateExpression(const Token & token) const {
    if (token.type == TokenType::StringLiteral) {
        return Value::fromString(token);
    }
    if (token.type == TokenType::IntLiteral) {
        try {
            return Value::fromInt(token);
        } catch (const std::invalid_argument & e) {
            throw std::runtime_error("Invalid integer literal: " + token.lexeme);
        } catch (const std::out_of_range & e) {
            throw std::runtime_error("Integer literal out of range: " + token.lexeme);
        }
    }
    if (token.type == TokenType::DoubleLiteral) {
        try {
            return Value::fromDouble(token);
        } catch (const std::invalid_argument & e) {
            throw std::runtime_error("Invalid double literal: " + token.lexeme);
        } catch (const std::out_of_range & e) {
            throw std::runtime_error("Double literal out of range: " + token.lexeme);
        }
    }
    if (token.type == TokenType::BooleanLiteral || token.type == TokenType::Identifier) {
        std::string lowered = token.lexeme;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return Value::fromBoolean(token, (lowered == "true" ? true : false));
    }
    if (token.type == TokenType::Variable) {
        return this->getVariable(token, this->contextPrefix, __FILE__, __LINE__);
    }
    THROW_UNEXPECTED_TOKEN_ERROR(token, "string, integer, double, or variable");

    return Value();
}

std::vector<Value> ScriptInterpreter::parseFunctionArguments(const std::vector<Token> & tokens,
                                                             std::size_t &              index) const {
    std::vector<Value> args;
    size_t             current_index = index;

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
    index = current_index;

    return args;
}

void ScriptInterpreter::handleBooleanDeclaration(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & varToken = tokens[i];
    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='

        if (i < tokens.size() && tokens[i].type == TokenType::Variable) {
            auto variable = this->getVariable(tokens[i], this->contextPrefix, __FILE__, __LINE__);

            if (variable.type != Variables::Type::VT_BOOLEAN) {
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varToken.lexeme,
                                                    Variables::TypeToString(Variables::Type::VT_BOOLEAN),
                                                    tokens[i].lexeme, variable.TypeToString(), tokens[i]);
            }
            this->setVariable(varToken.lexeme, variable, this->contextPrefix, true);
            i++;  // Skip variable name
            EXPECT_SEMICOLON(tokens, i, "after bool variable declaration");

        } else if (i < tokens.size() &&
                   (tokens[i].type == TokenType::Identifier || tokens[i].type == TokenType::StringLiteral)) {
            std::string lowered = tokens[i].lexeme;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (lowered == "true") {
                auto value = Value::fromBoolean(tokens[i], true);
                this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            } else if (lowered == "false") {
                auto value = Value::fromBoolean(tokens[i], false);
                this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            } else {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "true or false after '='");
            }
            i++;  // Skip boolean literal
            EXPECT_SEMICOLON(tokens, i, "after bool declaration");
        } else if (i < tokens.size() && tokens[i].type == TokenType::IntLiteral) {
            const auto test = std::stoi(tokens[i].lexeme);
            if (test == 0) {
                auto value = Value::fromBoolean(tokens[i], false);
                this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            } else if (test > 0) {
                auto value = Value::fromBoolean(tokens[i], false);
                this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            } else {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "bool literal after '='");
            }
            i++;
            EXPECT_SEMICOLON(tokens, i, "after bool declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "bool literal after '='");
        }

    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after bool declaration");
    }
};

void ScriptInterpreter::handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & varToken = tokens[i];

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='

        if (i < tokens.size() && tokens[i].type == TokenType::Variable) {
            const auto variable = this->getVariable(tokens[i], this->contextPrefix, __FILE__, __LINE__);

            if (variable.type != Variables::Type::VT_STRING) {
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varToken.lexeme,
                                                    Variables::TypeToString(Variables::Type::VT_STRING),
                                                    tokens[i].lexeme, variable.TypeToString(), tokens[i]);
            }
            auto value = variable;
            this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            i++;  // Skip variable name
            EXPECT_SEMICOLON(tokens, i, "after string variable declaration");

        } else if (i < tokens.size() && tokens[i].type == TokenType::StringLiteral) {
            auto value = Value::fromString(tokens[i]);
            this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
            i++;  // Skip string literal
            EXPECT_SEMICOLON(tokens, i, "after string declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "string literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after string declaration");
    }
}

void ScriptInterpreter::handleNumberDeclaration(const std::vector<Token> & tokens, std::size_t & i, TokenType type) {
    const auto & varToken = tokens[i];

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            if (type == TokenType::IntDeclaration && tokens[i].type == TokenType::IntLiteral) {
                try {
                    auto value = Value::fromInt(tokens[i]);
                    this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
                    i++;  // Skip int literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid integer literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Integer literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else if (type == TokenType::DoubleDeclaration && tokens[i].type == TokenType::DoubleLiteral) {
                try {
                    auto value = Value::fromDouble(tokens[i]);
                    this->setVariable(varToken.lexeme, value, this->contextPrefix, true);
                    i++;  // Skip double literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid double literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Double literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else {
                const std::string expectedType = type == TokenType::IntDeclaration ? "int" : "double";
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varToken.lexeme, expectedType, "",
                                                    getVariableTypeFromTokenTypeAsString(tokens[i].type), tokens[i]);
            }
            EXPECT_SEMICOLON(tokens, i, "after variable declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after variable declaration, variable name: " + varToken.lexeme);
    }
}

void ScriptInterpreter::handleFunctionDeclaration(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & funcToken = tokens[i];

    i++;  // skip funct name

    if (i < tokens.size() && tokens[i].type != TokenType::Equals) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after function declaration");
    }
    i++;  // skip '='

    if (this->functionBodies.find(funcToken.lexeme) != this->functionBodies.end()) {
        THROW_FUNCTION_REDEFINITION_ERROR(funcToken.lexeme, tokens[i]);
    }

    if (i < tokens.size() && tokens[i].type != TokenType::LeftParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "");
    }
    i++;
    // parse arg definitions
    const std::string context_name = this->getContextName(funcToken.lexeme);
    const auto        args = ScriptInterpreterHelpers::parseFunctionDeclarationArguments(tokens, i, __FILE__, __LINE__);
    for (const auto & arg : args) {
        auto value = arg;
        this->setVariable(arg.GetToken().lexeme, value, context_name, true);
    }

    size_t start;
    size_t end;
    ScriptInterpreterHelpers::getFunctionBody(tokens, i, start, end);

    const std::string function_body = ScriptInterpreterHelpers::extractSubstring(this->source, start, end);
    if (function_body.empty()) {
        std::cout << this->source << '\n';
        THROW_FUNCTION_BODY_EMPTY(funcToken.lexeme, tokens[i - 1]);
    }
    this->functionBodies[funcToken.lexeme] = function_body;
    // recheck the close curly brace
    if (i >= tokens.size() || tokens[i].type != TokenType::RightCurlyBracket) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "}");
    }
    i++;
}

void ScriptInterpreter::handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & functiontoken = tokens[i];

    i++;  // skip funct name
    if (i < tokens.size() && tokens[i].type != TokenType::LeftParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "");
    }

    std::vector<Value> args = parseFunctionArguments(tokens, i);

    auto it1 = functionObjects.find(functiontoken.lexeme);
    if (it1 != functionObjects.end()) {
        it1->second->call(args);
    } else {
        auto it2 = functionBodies.find(functiontoken.lexeme);
        if (it2 == functionBodies.end()) {
            THROW_UNDEFINED_FUNCTION_ERROR(functiontoken.lexeme, tokens[i]);
        }

        if (args.size() > 0) {
            auto varList = this->getcontextVariables(this->getContextName(functiontoken.lexeme));
            if (varList.size() != args.size()) {
                THROW_FUNCTION_ARG_COUNT_MISMATCH_ERROR(functiontoken.lexeme, varList.size(), args.size(), tokens[i]);
            }
            size_t counter = 0;
            for (auto & var : varList) {
                this->setVariable(var.first, args[counter], var.second.context, false, true);
                counter++;
            }
        }

        this->executeScript(it2->second, this->filename, functiontoken.lexeme, true);
    }

    //i++;

    EXPECT_SEMICOLON(tokens, i, "after function call");
}

void ScriptInterpreter::handleVariableReference(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & varToken = tokens[i];
    i++;      // Skip variable token to avoid infinite loop
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            const auto variable = this->getVariable(varToken, this->contextPrefix, __FILE__, __LINE__);
            auto       value    = evaluateExpression(tokens[i]);
            this->setVariable(varToken.lexeme, value, this->contextPrefix, false);
            i++;  // Skip value
            EXPECT_SEMICOLON(tokens, i, "after variable assignment");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "value after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "'=' for assignment");
    }
}

void ScriptInterpreter::executeScript(const std::string & source, const std::string & filename,
                                      const std::string & _namespace, bool ignore_tags) {
    std::string oldContext;
    std::string oldSource;

    this->filename = filename;
    if (!this->source.empty()) {
        oldSource = this->source;
    }
    this->source = source;

    if (!this->contextPrefix.empty()) {
        oldContext = this->contextPrefix;
    }
    this->contextPrefix = this->getContextName(_namespace);
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

        if (insideScript == false && ignore_tags == false) {
            //std::cout << token.lexeme;
            i++;
            continue;
        }

        switch (token.type) {
            case TokenType::StringDeclaration:
                handleStringDeclaration(tokens, i);
                break;
            case TokenType::BooleanDeclaration:
                handleBooleanDeclaration(tokens, i);
                break;
            case TokenType::FunctionDeclaration:
                handleFunctionDeclaration(tokens, i);
                break;
            case TokenType::IntDeclaration:
            case TokenType::DoubleDeclaration:
                handleNumberDeclaration(tokens, i, token.type);
                break;
            case TokenType::FunctionCall:
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
                THROW_UNEXPECTED_TOKEN_ERROR(token, "");
        }
    }
    if (!oldContext.empty()) {
        this->contextPrefix = oldContext;
        oldContext.clear();
    }
    if (!oldSource.empty()) {
        this->source = oldSource;
        oldSource.clear();
    }
}
