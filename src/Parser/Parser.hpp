#ifndef PARSER_HPP
#define PARSER_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "BaseException.hpp"
#include "Lexer/Token.hpp"
#include "Lexer/TokenType.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/Value.hpp"

namespace Parser {

class Parser {
  public:
    Parser() {}

    class Exception : public BaseException {
      public:
        using BaseException::BaseException;

        Exception(const std::string & msg, const std::string & expected, const Lexer::Tokens::Token & token) {
            rawMessage_ = msg + ": " + token.dump();
            context_ =
                " at line: " + std::to_string(token.line_number) + ", column: " + std::to_string(token.column_number);
            if (expected.empty() == false) {
                rawMessage_ += " (expected: " + expected + ")";
            }
            formattedMessage_ = formatMessage();
        }

        Exception(const std::string & msg, const std::string & expected, int line, int col) {
            rawMessage_ = msg;
            if (expected.empty() == false) {
                rawMessage_ += " (expected: " + expected + ")";
            }
            context_          = " at line: " + std::to_string(line) + ", column: " + std::to_string(col);
            formattedMessage_ = formatMessage();
        }

        std::string formatMessage() const override { return "[Syntax ERROR] >>" + context_ + " << : " + rawMessage_; }
    };

    void parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                     const std::string & filename);

    static const std::unordered_map<std::string, Lexer::Tokens::Type>              keywords;
    static const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> variable_types;

  private:
    std::vector<Lexer::Tokens::Token> tokens_;
    std::string_view                  input_str_view_;
    size_t                            current_token_index_;
    std::string                       current_filename_;

    // Token stream handling and error-reporting helper functions (unchanged)
    const Lexer::Tokens::Token & currentToken() const {
        if (isAtEnd()) {
            // Technically we should never reach this if parseScript's loop is correct
            // But it's useful as a safety check
            if (!tokens_.empty() && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE) {
            return tokens_.back();  // return the EOF token
            }
            throw std::runtime_error("Unexpected end of token stream reached.");
        }
        return tokens_[current_token_index_];
    }

    // Look ahead in the token stream
    const Lexer::Tokens::Token & peekToken(size_t offset = 1) const {
        if (current_token_index_ + offset >= tokens_.size()) {
            // If at or beyond EOF, return the last token (should be EOF)
            if (!tokens_.empty()) {
                return tokens_.back();
            }
            throw std::runtime_error("Cannot peek beyond end of token stream.");
        }
        return tokens_[current_token_index_ + offset];
    }

    // Consume (advance past) the current token and return it
    Lexer::Tokens::Token consumeToken() {
        if (isAtEnd()) {
            throw std::runtime_error("Cannot consume token at end of stream.");
        }
        return tokens_[current_token_index_++];
    }

    // Check if current token type matches the expected type
    // If so, consume it and return true; otherwise return false
    bool match(Lexer::Tokens::Type expected_type) {
        if (isAtEnd()) {
            return false;
        }
        if (currentToken().type == expected_type) {
            consumeToken();
            return true;
        }
        return false;
    }

    // Check if current token type and value match the expected ones
    // Only use value checking for operators and punctuation
    bool match(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
        if (isAtEnd()) {
            return false;
        }
        const auto & token = currentToken();
        if (token.type == expected_type && token.value == expected_value) {
            consumeToken();
            return true;
        }
        return false;
    }

    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type) {
        if (isAtEnd()) {
            reportError("Unexpected end of file, expected token type: " + Lexer::Tokens::TypeToString(expected_type));
        }
        const auto & token = currentToken();
        if (token.type == expected_type) {
            return consumeToken();
        }
        reportError("Expected token type " + Lexer::Tokens::TypeToString(expected_type));
        // reportError throws; this return is never reached, but may satisfy the compiler
        return token;  // or let reportError throw
    }

    // Like expect, but also checks the token's value
    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
        if (isAtEnd()) {
            reportError("Unexpected end of file, expected token: " + Lexer::Tokens::TypeToString(expected_type) +
                        " with value '" + expected_value + "'");
        }
        const auto & token = currentToken();
        if (token.type == expected_type && token.value == expected_value) {
            return consumeToken();
        }
        reportError("Expected token " + Lexer::Tokens::TypeToString(expected_type) + " with value '" + expected_value +
                    "'");
        return token;  // reportError throws
    }

    // Check if we've reached the end of relevant tokens (just before EOF)
    bool isAtEnd() const {
        // We're at the end if the index equals the number of tokens,
        // or if only the EOF token remains (as the last element)
        return current_token_index_ >= tokens_.size() ||
               (current_token_index_ == tokens_.size() - 1 && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE);
    }

    [[noreturn]] void reportError(const std::string & message, const std::string& expected = "") {
        if (current_token_index_ < tokens_.size()) {
            throw Exception(message, expected, tokens_[current_token_index_]);
        }
        int line = tokens_.empty() ? 0 : tokens_.back().line_number;
        int col  = tokens_.empty() ? 0 : tokens_.back().column_number;
        throw Exception(message, expected, line, col);
    }

    // parseStatement (unchanged)
    void parseStatement() {
        const auto & token_type = currentToken().type;

        if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
            parseFunctionDefinition();
            return;
        }

        // Variable definition if leading token matches a type keyword
        if (Parser::variable_types.find(token_type) != Parser::variable_types.end()) {
            parseVariableDefinition();
            return;
        }

        reportError("Unexpected token at beginning of statement");
    }

    void parseVariableDefinition();
    void parseFunctionDefinition();

    // --- Parsing helper functions ---

    // type : KEYWORD_STRING | KEYWORD_INT | KEYWORD_DOUBLE
    // Returns the corresponding Symbols::Variables::Type enum and consumes the token
    Symbols::Variables::Type parseType() {
        const auto & token = currentToken();
        // Direct lookup for type keyword
        auto it = Parser::variable_types.find(token.type);
        if (it != Parser::variable_types.end()) {
            consumeToken();
            return it->second;
        }
        reportError("Expected type keyword (string, int, double, float)");
    }

    Symbols::Value parseValue(Symbols::Variables::Type expected_var_type) {
        Lexer::Tokens::Token token       = currentToken();
        bool                 is_negative = false;

    // Handle unary sign
        if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && (token.lexeme == "-" || token.lexeme == "+") &&
            peekToken().type == Lexer::Tokens::Type::NUMBER) {
            is_negative = (token.lexeme == "-");
            token       = peekToken();
            consumeToken();  // consumed the sign
        }

        // STRING type
        if (expected_var_type == Symbols::Variables::Type::STRING) {
            if (token.type == Lexer::Tokens::Type::STRING_LITERAL) {
                consumeToken();
                return Symbols::Value(token.value);
            }
            reportError("Expected string literal value");
        }

        // BOOLEAN type
        if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
            if (token.type == Lexer::Tokens::Type::KEYWORD && (token.value == "true" || token.value == "false")) {
                consumeToken();
                return Symbols::Value(token.value == "true");
            }
            reportError("Expected boolean literal value (true or false)");
        }

        // NUMERIC types
        if (expected_var_type == Symbols::Variables::Type::INTEGER ||
            expected_var_type == Symbols::Variables::Type::DOUBLE ||
            expected_var_type == Symbols::Variables::Type::FLOAT) {
            if (token.type == Lexer::Tokens::Type::NUMBER) {
                Symbols::Value val = parseNumericLiteral(token.value, is_negative, expected_var_type);
                consumeToken();
                return val;
            }

            reportError("Expected numeric literal value");
        }

        reportError("Unsupported variable type encountered during value parsing");
        return Symbols::Value();  // compiler happy
    }

    Symbols::Value parseNumericLiteral(const std::string & value, bool is_negative, Symbols::Variables::Type type);

    void parseFunctionBody(const Lexer::Tokens::Token & opening_brace, const std::string & function_name,
                           Symbols::Variables::Type return_type, const Symbols::FunctionParameterInfo & params);

    ParsedExpressionPtr parseParsedExpression(const Symbols::Variables::Type & expected_var_type);

};  // class Parser

}  // namespace Parser

#endif  // PARSER_HPP
