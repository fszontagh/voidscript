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
    if (token.type == TokenType::Variable) {
        return this->getVariable(token);
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
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;
    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='

        if (i < tokens.size() && tokens[i].type == TokenType::Variable) {
            const auto variable = this->getVariable(tokens[i]);

            if (variable.type != Variables::Type::VT_BOOLEAN) {
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName, Variables::TypeToString(Variables::Type::VT_BOOLEAN),
                                                    tokens[i].lexeme, variable.TypeToString(), tokens[i]);
            }
            this->setVariable(varName, variable);
            i++;  // Skip variable name
            EXPECT_SEMICOLON(tokens, i, "after bool variable declaration");

        } else if (i < tokens.size() && tokens[i].type == TokenType::Identifier) {
            std::string lowered = tokens[i].lexeme;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (lowered == "true") {
                this->setVariable(varName, Value::fromBoolean(tokens[i], true));
            } else if (lowered == "false") {
                this->setVariable(varName, Value::fromBoolean(tokens[i], false));
            } else {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "true or false after '='");
            }
            i++;  // Skip boolean literal
            EXPECT_SEMICOLON(tokens, i, "after bool declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "bool literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after bool declaration");
    }
};

void ScriptInterpreter::handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i) {
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='

        if (i < tokens.size() && tokens[i].type == TokenType::Variable) {
            const auto variable = this->getVariable(tokens[i]);

            if (variable.type != Variables::Type::VT_STRING) {
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName, Variables::TypeToString(Variables::Type::VT_STRING),
                                                    tokens[i].lexeme, variable.TypeToString(), tokens[i]);
            }
            this->setVariable(varName, variable);
            //variables[varName] = variables[tokens[i].lexeme];
            i++;  // Skip variable name
            EXPECT_SEMICOLON(tokens, i, "after string variable declaration");

        } else if (i < tokens.size() && tokens[i].type == TokenType::StringLiteral) {
            this->setVariable(varName, Value::fromString(tokens[i]));
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
    const auto varName = tokens[i].lexeme;
    const auto varType = tokens[i].variableType;

    i++;      // Skip variable name
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            if (type == TokenType::IntDeclaration && tokens[i].type == TokenType::IntLiteral) {
                try {
                    const auto variable = this->getVariable(tokens[i]);
                    if (variable.type != Variables::Type::VT_INT) {
                        THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName, Variables::TypeToString(Variables::Type::VT_INT),
                                                            tokens[i].lexeme, variable.TypeToString(), tokens[i]);
                    }
                    this->setVariable(varName, Value::fromInt(tokens[i]));
                    i++;  // Skip int literal
                } catch (const std::invalid_argument & e) {
                    throw std::runtime_error("Invalid integer literal in declaration: " + tokens[i].lexeme);
                } catch (const std::out_of_range & e) {
                    throw std::runtime_error("Integer literal out of range in declaration: " + tokens[i].lexeme);
                }
            } else if (type == TokenType::DoubleDeclaration && tokens[i].type == TokenType::DoubleLiteral) {
                try {
                    const auto variable = this->getVariable(tokens[i]);
                    if (variable.type != Variables::Type::VT_DOUBLE) {
                        THROW_VARIABLE_TYPE_MISSMATCH_ERROR(varName,
                                                            Variables::TypeToString(Variables::Type::VT_DOUBLE),
                                                            tokens[i].lexeme, variable.TypeToString(), tokens[i]);
                    }
                    this->setVariable(varName, Value::fromDouble(tokens[i]));
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
            EXPECT_SEMICOLON(tokens, i, "after variable declaration");
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[i - 1], "literal after '='");
        }
    } else {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "= after variable declaration, variable name: " + varName);
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

    const auto args = ScriptInterpreterHelpers::parseFunctionDeclarationArguments(tokens, i);
    std::cout << "args: " << args.size() << std::endl;
    for (const auto & arg : args) {
        std::cout << "arg name: " << arg.GetToken().lexeme << " type: " << arg.TypeToString() << std::endl;
    }
    this->functionParameters[varName].assign(args.begin(), args.end());
    const std::string body        = ScriptInterpreterHelpers::getFunctionBody(tokens, i);
    this->functionBodies[varName] = body;
    // recheck the close curly brace
    if (i >= tokens.size() || tokens[i].type != TokenType::RightCurlyBracket) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "}");
    }
    i++;
    //this->functionBodies[varName] = std::make_shared<ScriptInterpreter>();
    //this->functionBodies[varName]->executeScript(body, this->filename, true);
    //EXPECT_SEMICOLON(tokens, i, "after function declaration");
    // there is no need semicolon to the end of the function declaration
    std::cout << "function body: \n\"" << body << "\"" << std::endl;
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
    //THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "function call or variable assignment (not yet implemented)");
//    const auto varName = tokens[i].lexeme;
//    const auto varType = tokens[i].variableType;
    const auto& varToken = tokens[i];
    i++;      // Skip variable token to avoid infinite loop
    if (i < tokens.size() && tokens[i].type == TokenType::Equals) {
        i++;  // Skip '='
        if (i < tokens.size()) {
            const auto variable = this->getVariable(varToken);
            this->setVariable(varToken.lexeme, evaluateExpression(tokens[i]));
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

void ScriptInterpreter::executeScript(const std::string & source, const std::string & filename, bool ignore_tags) {
    this->filename = filename;
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
