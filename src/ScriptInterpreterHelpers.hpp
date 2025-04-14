#ifndef SCRIPTINTERPRETERHELPERS_HPP
#define SCRIPTINTERPRETERHELPERS_HPP

#include <iostream>
#include <ostream>
#include <vector>

#include "ScriptExceptionMacros.h"
#include "Token.hpp"
#include "Value.hpp"

#define EXPECT_SEMICOLON(tokens, i, message) \
    ScriptInterpreterHelpers::expectSemicolon(tokens, i, message, __FILE__, __LINE__)

namespace ScriptInterpreterHelpers {

static std::string extractSubstring(const std::string & str, const size_t & start, const size_t & end) {
    if (start >= 0 && start < str.length() && end >= start && end < str.length()) {
        return str.substr(start, end - start + 1);
    }
    return "";
}

static void expectSemicolon(const std::vector<Token> & tokens, std::size_t & i, const std::string & message,
                            const std::string & file = __FILE__, int line = __LINE__) {
    if (i >= tokens.size() || tokens[i].type != TokenType::Semicolon) {
        THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i - 1], "; " + message, file, line);
    }
    i++;  // Skip ';'
}

[[nodiscard]] static std::vector<Value> parseFunctionDeclarationArguments(const std::vector<Token> & tokens,
                                                                          std::size_t &              i,
                                                                          const std::string &        file = __FILE__,
                                                                          int                        line = __LINE__) {
    std::vector<Value> arguments;

    // check the arguments types
    if (tokens[i].type != TokenType::StringDeclaration && tokens[i].type != TokenType::BooleanDeclaration &&
        tokens[i].type != TokenType::IntDeclaration && tokens[i].type != TokenType::DoubleDeclaration &&
        tokens[i].type != TokenType::RightParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i], "variable declaration", file, line);
    }
    if (tokens[i].type != TokenType::RightParenthesis) {
        const auto parameter_type = getVariableTypeFromTokenTypeDeclaration(tokens[i].type);
        if (parameter_type == Variables::Type::VT_NOT_DEFINED) {
            THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i], "valid type identifier", file, line);
        }

        if (parameter_type == Variables::Type::VT_FUNCTION) {
            THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i], "valid type identifier", file, line);
        }

        if (parameter_type == Variables::Type::VT_NULL) {
            THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i], "valid type identifier", file, line);
        }

        Value val;
        val.type  = parameter_type;
        val.token = tokens[i];

        arguments.emplace_back(std::move(val));
        i++;  // Skip variable declaration
    }

    if (tokens[i].type != TokenType::RightParenthesis) {
        THROW_UNEXPECTED_TOKEN_ERROR_HELPER(tokens[i], ") - Only one argument is allowed", file, line);
    }
    i++;  // Skip ')'

    return arguments;
}

static void getFunctionBody(const std::vector<Token> & tokens, std::size_t & i, std::size_t & start,
                            std::size_t & end) {
    start = tokens[i].pos.end;
    std::cout << "START Token: " << tokens[i].lexeme << " start pos: " << std::to_string(tokens[i].pos.start)
              << " end pos: " << std::to_string(tokens[i].pos.end) << '\n';

    if (i >= tokens.size() || tokens[i].type != TokenType::LeftCurlyBracket) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "{");
    }
    i++;  // Skip '{'

    while (i < tokens.size() && tokens[i].type != TokenType::RightCurlyBracket) {
        if (tokens[i].type == TokenType::EndOfLine) {
            i++;
            continue;
        }
        if (tokens[i].type == TokenType::EndOfFile) {
            throw std::runtime_error("Unexpected end of file");
            break;
        }
        i++;
    }
    end = tokens[i].pos.start - 1;

    std::cout << "END Token: " << tokens[i].lexeme << " start pos: " << std::to_string(tokens[i].pos.start)
              << " end pos: " << std::to_string(tokens[i].pos.end) << '\n';

    if (i >= tokens.size() || tokens[i].type != TokenType::RightCurlyBracket) {
        THROW_UNEXPECTED_TOKEN_ERROR(tokens[i], "}");
    }
};

};  // namespace ScriptInterpreterHelpers

#endif  // SCRIPTINTERPRETERHELPERS_HPP
