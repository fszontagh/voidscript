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

    //   if (current_index >= tokens.size() || tokens[current_index].type != TokenType::Identifier) {
    //       THROW_UNEXPECTED_TOKEN_ERROR(tokens[current_index], tokenTypeNames.at(TokenType::Identifier));
    //   }
    //   current_index++;  // Skip function name

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
            const auto variable = this->getVariable(tokens[i], this->contextPrefix, __FILE__, __LINE__);

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
                this->setVariable(varToken.lexeme, Value::fromBoolean(tokens[i], true), this->contextPrefix, true);
            } else if (lowered == "false") {
                this->setVariable(varToken.lexeme, Value::fromBoolean(tokens[i], false), this->contextPrefix, true);
            } else {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "true or false after '='");
            }
            i++;  // Skip boolean literal
            EXPECT_SEMICOLON(tokens, i, "after bool declaration");
        } else if (i < tokens.size() && tokens[i].type == TokenType::IntLiteral) {
            const auto test = std::stoi(tokens[i].lexeme);
            if (test == 0) {
                this->setVariable(varToken.lexeme, Value::fromBoolean(tokens[i], false), this->contextPrefix, true);
            } else if (test > 0) {
                this->setVariable(varToken.lexeme, Value::fromBoolean(tokens[i], true), this->contextPrefix, true);
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
    const auto varToken = tokens[i];

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
            this->setVariable(varToken.lexeme, variable, this->contextPrefix, true);
            i++;  // Skip variable name
            EXPECT_SEMICOLON(tokens, i, "after string variable declaration");

        } else if (i < tokens.size() && tokens[i].type == TokenType::StringLiteral) {
            this->setVariable(varToken.lexeme, Value::fromString(tokens[i]), this->contextPrefix, true);
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
                    this->setVariable(varToken.lexeme, Value::fromInt(tokens[i]), this->contextPrefix, true);
                    i++;  // Skip int literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid integer literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Integer literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else if (type == TokenType::DoubleDeclaration && tokens[i].type == TokenType::DoubleLiteral) {
                try {
                    this->setVariable(varToken.lexeme, Value::fromDouble(tokens[i]), this->contextPrefix, true);
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
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;

    i++;  // skip funct name

    if (i < tokens.size() && tokens[i].type != TokenType::Equals) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after function declaration");
    }
    i++;  // skip '='

    if (this->functionParameters.find(varName) != this->functionParameters.end()) {
        THROW_FUNCTION_REDEFINITION_ERROR(varName, tokens[i]);
    }

    if (i < tokens.size() && tokens[i].type != TokenType::LeftParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "");
    }
    i++;
    // parse arg definitions

    const auto args = ScriptInterpreterHelpers::parseFunctionDeclarationArguments(tokens, i, __FILE__, __LINE__);
    std::cout << "args: " << args.size() << std::endl;
    for (const auto & arg : args) {
        std::cout << "arg name: " << arg.GetToken().lexeme << " type: " << arg.TypeToString() << std::endl;
    }
    this->functionParameters[varName].assign(args.begin(), args.end());
    size_t start;
    size_t end;
    ScriptInterpreterHelpers::getFunctionBody(tokens, i, start, end);
    std::cout << "Body start:  " << start << " end: " << end << std::endl;
    const std::string function_body = ScriptInterpreterHelpers::extractSubstring(this->source, start, end);
    this->functionBodies[varName]   = function_body;
    // recheck the close curly brace
    if (i >= tokens.size() || tokens[i].type != TokenType::RightCurlyBracket) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "}");
    }
    i++;
#if DEBUG_BUILD == 1
    std::cout << "function body: \n\"" << function_body << "\"" << std::endl;
#endif
}

void ScriptInterpreter::handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i) {
    auto        index    = i;
    std::string funcName = tokens[i].lexeme;

    index++;  // skip funct name
    if (index < tokens.size() && tokens[index].type != TokenType::LeftParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[index - 1], "");
    }

    auto it = functionObjects.find(funcName);
    if (it == functionObjects.end()) {
        THROW_UNDEFINED_FUNCTION_ERROR(funcName, tokens[i]);
    }

    //    it->second->validate(tokens, i, this->variables);

    std::vector<Value> args = parseFunctionArguments(tokens, index);
    it->second->call(args);
    i = index;

    EXPECT_SEMICOLON(tokens, i, "after function call");

    //    if (i < tokens.size() && tokens[i].type == TokenType::Semicolon) {
    //        i++;  // Skip ';' after function call
    //    }
}

void ScriptInterpreter::handleVariableReference(const std::vector<Token> & tokens, std::size_t & i) {
    const auto & varToken = tokens[i];
    i++;      // Skip variable token to avoid infinite loop
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            const auto variable = this->getVariable(varToken, this->contextPrefix, __FILE__, __LINE__);
            this->setVariable(varToken.lexeme, evaluateExpression(tokens[i]), this->contextPrefix, false);
            i++;  // Skip value
            EXPECT_SEMICOLON(tokens, i, "after variable assignment");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "value after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "'=' for assignment");
    }
}

void ScriptInterpreter::handleComment(std::size_t & i) {
    i++;  // Skip comment token
}

void ScriptInterpreter::handleSemicolon(std::size_t & i) {
    i++;  // Skip semicolon token
}

void ScriptInterpreter::executeScript(const std::string & source, const std::string & filename,
                                      const std::string & _namespace, bool ignore_tags) {
    this->filename      = filename;
    this->source        = source;
    this->contextPrefix = filename + _namespace;
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
                THROW_UNEXPECTED_TOKEN_ERROR(token, "");
        }
    }
}
