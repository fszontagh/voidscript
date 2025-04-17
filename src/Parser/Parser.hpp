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

        Exception(const std::string & msg, const Lexer::Tokens::Token & token) {
            rawMessage_ = msg + ": " + token.dump();
            context_ =
                " at line: " + std::to_string(token.line_number) + ", column: " + std::to_string(token.column_number);
            formattedMessage_ = formatMessage();
        }

        Exception(const std::string & msg, int line, int col) {
            rawMessage_       = msg;
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

    // Token Stream Kezelő és Hibakezelő segédfüggvények (változatlanok)
    const Lexer::Tokens::Token & currentToken() const {
        if (isAtEnd()) {
            // Technikailag itt már nem kellene lennünk, ha a parseProgram ciklus jól van megírva
            // De biztonsági ellenőrzésként jó lehet
            if (!tokens_.empty() && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE) {
                return tokens_.back();  // Visszaadjuk az EOF tokent
            }
            throw std::runtime_error("Unexpected end of token stream reached.");
        }
        return tokens_[current_token_index_];
    }

    // Előre néz a token stream-ben
    const Lexer::Tokens::Token & peekToken(size_t offset = 1) const {
        if (current_token_index_ + offset >= tokens_.size()) {
            // EOF vagy azon túl vagyunk, adjuk vissza az utolsó tokent (ami EOF kell legyen)
            if (!tokens_.empty()) {
                return tokens_.back();
            }
            throw std::runtime_error("Cannot peek beyond end of token stream.");
        }
        return tokens_[current_token_index_ + offset];
    }

    // Elfogyasztja (lépteti az indexet) az aktuális tokent és visszaadja azt
    Lexer::Tokens::Token consumeToken() {
        if (isAtEnd()) {
            throw std::runtime_error("Cannot consume token at end of stream.");
        }
        return tokens_[current_token_index_++];
    }

    // Ellenőrzi, hogy az aktuális token típusa megegyezik-e a várttal.
    // Ha igen, elfogyasztja és true-t ad vissza. Ha nem, false-t ad vissza.
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

    // Ellenőrzi, hogy az aktuális token típusa és értéke megegyezik-e a várttal.
    // Csak OPERATOR és PUNCTUATION esetén érdemes használni az érték ellenőrzést.
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
        // A reportError dob, ez a return sosem fut le, de a fordító kedvéért kellhet:
        return token;  // Vagy dobjon a reportError
    }

    // Mint az expect, de az értékét is ellenőrzi.
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
        return token;  // reportError dob
    }

    // Ellenőrzi, hogy a releváns tokenek végére értünk-e (az EOF előtti utolsó tokenen vagyunk-e)
    bool isAtEnd() const {
        // Akkor vagyunk a végén, ha az index a tokenek méretével egyenlő,
        // vagy ha már csak az EOF token van hátra (ha az a lista utolsó eleme).
        return current_token_index_ >= tokens_.size() ||
               (current_token_index_ == tokens_.size() - 1 && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE);
    }

    [[noreturn]] void reportError(const std::string & message) {
        if (current_token_index_ < tokens_.size()) {
            throw Exception(message, tokens_[current_token_index_]);
        }
        int line = tokens_.empty() ? 0 : tokens_.back().line_number;
        int col  = tokens_.empty() ? 0 : tokens_.back().column_number;
        throw Exception(message, line, col);
    }

    // parseStatement (változatlan)
    void parseStatement() {
        const auto & token_type = currentToken().type;

        if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
            parseFunctionDefinition();
            return;
        }

        for (const auto & _type : Parser::Parser::variable_types) {
            if (token_type == _type.first) {
                parseVariableDefinition();
                return;
            }
        }

        reportError("Unexpected token at beginning of statement");
    }

    void parseVariableDefinition();
    void parseFunctionDefinition();

    // --- Elemzési Segédfüggvények ---

    // type : KEYWORD_STRING | KEYWORD_INT | KEYWORD_DOUBLE
    // Visszaadja a megfelelő Symbols::Variables::Type enum értéket és elfogyasztja a tokent.
    Symbols::Variables::Type parseType() {
        const auto & token = currentToken();
        for (const auto & _type : Parser::variable_types) {
            if (token.type == _type.first) {
                consumeToken();
                return _type.second;
            }
        }

        reportError("Expected type keyword (string, int, double, float)");
    }

    Symbols::Value parseValue(Symbols::Variables::Type expected_var_type) {
        Lexer::Tokens::Token token       = currentToken();
        bool                 is_negative = false;

        // Előjel kezelése
        if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && (token.lexeme == "-" || token.lexeme == "+") &&
            peekToken().type == Lexer::Tokens::Type::NUMBER) {
            is_negative = (token.lexeme == "-");
            token       = peekToken();
            consumeToken();  // előjelet elfogyasztottuk
        }

        // STRING típus
        if (expected_var_type == Symbols::Variables::Type::STRING) {
            if (token.type == Lexer::Tokens::Type::STRING_LITERAL) {
                consumeToken();
                return Symbols::Value(token.value);
            }
            reportError("Expected string literal value");
        }

        // BOOLEAN típus
        if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
            if (token.type == Lexer::Tokens::Type::KEYWORD && (token.value == "true" || token.value == "false")) {
                consumeToken();
                return Symbols::Value(token.value == "true");
            }
            reportError("Expected boolean literal value (true or false)");
        }

        // NUMERIC típusok
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
