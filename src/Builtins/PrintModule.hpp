#ifndef PRINTFUNCTION_HPP
#define PRINTFUNCTION_HPP

#include <iostream>

#include "BaseFunction.hpp"
#include "ScriptExceptionMacros.h"
#include "ScriptInterpreter.hpp"
#include "Token.hpp"
#include "Value.hpp"

class PrintFunction : public BaseFunction {
  private:
    const std::string name = "print";
  public:
    void validate(const std::vector<Token> & tokens, size_t & i) const override {
        auto index = i;
        if (tokens[index].type != TokenType::Identifier) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "identifier: " + name);
        }
        index++;  // skip function name
        if (tokens[index].type != TokenType::LeftParenthesis) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "('");
        }
        index++;  // skip '('
        if (tokens[index].type != TokenType::StringLiteral && tokens[index].type != TokenType::Variable &&
            tokens[index].type != TokenType::IntLiteral && tokens[index].type != TokenType::DoubleLiteral) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "string, int, double or variable as argument");
        }
        size_t count = 0;
        while (tokens[index].type != TokenType::RightParenthesis) {
            if (tokens[index].type == TokenType::StringLiteral || tokens[index].type == TokenType::Variable ||
                tokens[index].type == TokenType::IntLiteral || tokens[index].type == TokenType::DoubleLiteral) {
                count++;
                index++;
            } else if (tokens[index].type == TokenType::Comma) {
                index++;
            } else {
                THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "string, int, double or variable as argument");
            }
        }
        if (count == 0) {
            throw std::runtime_error("print() requires at least one argument at");
        }
        index++;  // skip ')'
        if (tokens[index].type == TokenType::Semicolon) {
            index++;
        } else {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], ";");
        }
    }

    Value call(const std::vector<Value> & args, bool debug = false) const override {
        for (const auto & arg : args) {
            std::cout << arg.ToString();
        }
        return Value();
    }
};

#endif  // PRINTFUNCTION_HPP
