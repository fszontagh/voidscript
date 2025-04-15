#ifndef LEXER_HPP
#define LEXER_HPP

#include <algorithm>    // std::find_if
#include <cctype>       // <<< Hozzáadva
#include <string>
#include <string_view>  // <<< Hozzáadva
#include <unordered_map>
#include <vector>

#include "Token.hpp"  // Feltételezzük, hogy ez a fenti Token.hpp

namespace Lexer {
class Lexer {
  public:
    Lexer() {
        for (const auto & vecRef :
             { std::cref(OPERATOR_ARITHMETIC), std::cref(OPERATOR_RELATIONAL), std::cref(OPERATOR_INCREMENT),
               std::cref(OPERATOR_ASSIGNMENT), std::cref(OPERATOR_LOGICAL), std::cref(PUNCTUATION) }) {
            for (const auto & str : vecRef.get()) {
                operators_ += str;
            }
        }

        operators_ += "$";
    }

    void addNamespaceInput(const std::string & ns, const std::string & input) {
        inputs_[ns]         = input;
        positions_[ns]      = 0;
        line_numbers_[ns]   = 1;
        column_numbers_[ns] = 1;
    }

    void setNamespace(const std::string & ns) { current_namespace_ = ns; }

    std::vector<Tokens::Token> tokenizeNamespace(const std::string & ns) {
        if (inputs_.find(ns) == inputs_.end()) {
            return {};
        }

        setNamespace(ns);
        std::vector<Tokens::Token> tokens;
        Tokens::Token              token;
        do {
            token = nextToken();
            tokens.push_back(token);
        } while (token.type != Tokens::Type::END_OF_FILE);

        tokens_[ns] = tokens;
        return tokens;
    }

    std::vector<Tokens::Token> getTokens(const std::string & ns) const {
        auto it = tokens_.find(ns);
        if (it != tokens_.end()) {
            return it->second;
        }
        return {};
    }

    void setKeyWords(const std::unordered_map<std::string, Tokens::Type> & new_keywords) { keywords = new_keywords; }

    Tokens::Token nextToken() {
        skipWhitespaceAndComments();
        size_t start = pos();

        if (isAtEnd()) {
            return createToken(Tokens::Type::END_OF_FILE, start, start);
        }

        char c = peek();
        if (isalpha(c) || c == '_') {
            return matchIdentifierOrKeyword(start);
        }
        if (isdigit(c) || (isdigit(c) && peek(1) == '.')|| (c == '.' && isdigit(peek(1)))) {
            return matchNumber(start);
        }
        if (c == '"' || c == '\'') {
            return matchStringLiteral(start);
        }
        if (operators_.find(c) != std::string_view::npos) {
            return matchOperatorOrPunctuation(start);
        }

        advance();
        return createToken(Tokens::Type::UNKNOWN, start, pos());
    }


  private:
    std::unordered_map<std::string, std::string>                inputs_;
    std::unordered_map<std::string, std::vector<Tokens::Token>> tokens_;
    std::unordered_map<std::string, size_t>                     positions_;
    std::unordered_map<std::string, int>                        line_numbers_;
    std::unordered_map<std::string, int>                        column_numbers_;

    std::string                                   operators_;
    std::string                                   current_namespace_;
    std::unordered_map<std::string, Tokens::Type> keywords;

    // two chars
    static const std::vector<std::string> OPERATOR_RELATIONAL;
    static const std::vector<std::string> OPERATOR_INCREMENT;
    static const std::vector<std::string> OPERATOR_ASSIGNMENT;
    static const std::vector<std::string> OPERATOR_LOGICAL;

    // one char
    static const std::vector<std::string> OPERATOR_ARITHMETIC;
    static const std::vector<std::string> PUNCTUATION;

    const std::string & input() const { return inputs_.at(current_namespace_); }

    size_t & pos() { return positions_[current_namespace_]; }

    int & line() { return line_numbers_[current_namespace_]; }

    int & col() { return column_numbers_[current_namespace_]; }

    Tokens::Token createToken(Tokens::Type type, size_t start, size_t end, const std::string & value = "") {
        Tokens::Token token;
        token.type          = type;
        token.start_pos     = start;
        token.end_pos       = end;
        token.line_number   = line();
        token.column_number = col();
        if (start <= end && end <= input().length()) {
            token.lexeme = std::string_view(input()).substr(start, end - start);
            token.value  = value.empty() ? std::string(token.lexeme) : value;
        }
        return token;
    }

    // --------------------------------------

    char peek(size_t offset = 0) const {
        const auto & in = inputs_.at(current_namespace_);
        size_t       cp = positions_.at(current_namespace_);
        if (cp + offset >= in.length()) {
            return '\0';
        }
        return in[cp + offset];
    }

    char advance() {
        char c = peek();
        pos()++;
        if (c == '\n') {
            line()++;
            col() = 1;
        } else {
            col()++;
        }
        return c;
    }

    bool isAtEnd() const { return positions_.at(current_namespace_) >= inputs_.at(current_namespace_).length(); }

    bool isComment(const char current_char) const {
        return (current_char == '/' && peek(1) == '/' || current_char == '#');
    }

    void skipWhitespaceAndComments() {
        while (!isAtEnd()) {
            char c = peek();
            if (isspace(c)) {
                advance();
            } else if ((c == '/' && peek(1) == '/') || c == '#') {
                while (!isAtEnd() && peek() != '\n') {
                    advance();
                }
            } else {
                break;
            }
        }
    }

    Tokens::Token matchIdentifierOrKeyword(size_t start_pos, Tokens::Type type = Tokens::Type::IDENTIFIER) {
        while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {
            advance();
        }
        size_t      end   = pos();
        std::string value = input().substr(start_pos, end - start_pos);
        if (value.empty()) {
            return createToken(Tokens::Type::UNKNOWN, start_pos, end);
        }

        if (type == Tokens::Type::IDENTIFIER) {
            auto it = keywords.find(value);
            if (it != keywords.end()) {
                return createToken(it->second, start_pos, end);
            }
        }
        return createToken(type, start_pos, end);
    }

    Tokens::Token matchNumber(size_t start_pos) {
        bool has_dot = false;

        while (!isAtEnd()) {
            if (isdigit(peek())) {
                advance();
            } else if (!has_dot && peek() == '.' && isdigit(peek(1))) {
                has_dot = true;
                advance();  // a pont
                advance();  // az első számjegy a pont után
            } else {
                break;
            }
        }

        size_t end = pos();
        return createToken(Tokens::Type::NUMBER, start_pos, end);
    }

    Tokens::Token matchStringLiteral(size_t start_pos) {
        char opening_quote = peek();
        advance();  // Skip opening quote
        std::string value;
        bool        unterminated = false;

        while (!isAtEnd()) {
            char c = peek();
            if (c == opening_quote) {
                advance();
                break;
            }
            if (c == '\\') {
                advance();
                char e = advance();
                switch (e) {
                    case 'n':
                        value += '\n';
                        break;
                    case 't':
                        value += '\t';
                        break;
                    case '"':
                        value += opening_quote;
                        break;
                    case '\\':
                        value += '\\';
                        break;
                    default:
                        value += e;
                        break;
                }
            } else {
                value += advance();
            }
        }

        size_t end = pos();
        if (unterminated) {
            return createToken(Tokens::Type::UNKNOWN, start_pos, end, input().substr(start_pos, end - start_pos));
        }
        return createToken(Tokens::Type::STRING_LITERAL, start_pos, end, value);
    }

    Tokens::Token matchOperatorOrPunctuation(size_t start_pos) {
        char first_char = advance();  // Első karakter elfogyasztása

        if (!isAtEnd()) {
            char        second_char = peek(0);  // Következő karakter megnézése
            std::string two_chars_str{ first_char, second_char };

            const std::vector<std::pair<const std::vector<std::string> *, Tokens::Type>> two_char_op_types = {
                { &OPERATOR_RELATIONAL, Tokens::Type::OPERATOR_RELATIONAL },
                { &OPERATOR_INCREMENT,  Tokens::Type::OPERATOR_INCREMENT  },
                { &OPERATOR_ASSIGNMENT, Tokens::Type::OPERATOR_ASSIGNMENT },
                { &OPERATOR_LOGICAL,    Tokens::Type::OPERATOR_LOGICAL    }
            };

            for (const auto & [vec_ptr, type] : two_char_op_types) {
                if (matchFromVector(*vec_ptr, two_chars_str)) {
                    advance();  // Második karakter elfogyasztása
                    size_t end_pos = pos();
                    return createToken(type, start_pos, end_pos);
                }
            }
        }

        // Egykarakteres operátor vagy írásjel
        std::string single_char_str(1, first_char);

        if (single_char_str == "$") {
            if (isalpha(peek(0)) || peek(0) == '_') {
                return matchIdentifierOrKeyword(start_pos, Tokens::Type::VARIABLE_IDENTIFIER);
            }
        }

        const std::vector<std::pair<const std::vector<std::string> *, Tokens::Type>> one_char_op_types = {
            { &OPERATOR_ARITHMETIC, Tokens::Type::OPERATOR_ARITHMETIC },
            { &OPERATOR_ASSIGNMENT, Tokens::Type::OPERATOR_ASSIGNMENT }, // "=" itt van!
            { &PUNCTUATION,         Tokens::Type::PUNCTUATION         }
        };

        for (const auto & [vec_ptr, type] : one_char_op_types) {
            if (matchFromVector(*vec_ptr, single_char_str)) {
                size_t end_pos = pos();
                return createToken(type, start_pos, end_pos);
            }
        }

        size_t end_pos = pos();
        return createToken(Tokens::Type::UNKNOWN, start_pos, end_pos);
    }

    static bool matchFromVector(const std::vector<std::string> & vec, const std::string & value) {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

};  // class Lexer

};  // namespace Lexer
#endif  // LEXER_HPP
