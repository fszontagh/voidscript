#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <iostream>
#include <string>
#include <string_view>

#include "Lexer/TokenType.hpp"

namespace Lexer::Tokens {

struct Token {
    Lexer::Tokens::Type type;
    std::string         value;      // A token feldolgozott értéke
    std::string_view    lexeme;     // A nyers szövegrész az eredeti stringből
    size_t              start_pos;  // Kezdő index
    size_t              end_pos;    // Vég index (exclusive)
    int                 line_number;
    int                 column_number;

    // Módosított print metódus a lexeme kiírásával
    void print() const {
        std::cout << "Token { Type: " << Lexer::Tokens::TypeToString(type) << ", Value: \"" << value << "\""
                  << ", Pos: [" << start_pos << ", " << end_pos
                  << ")"
                  // Lexeme kiírása (stringgé konvertálva a biztonság kedvéért, ha pl. null lenne)
                  << ", Lexeme: \"" << std::string(lexeme) << "\""
                  << " }" << '\n';
    }

    std::string dump() const {
        return + "Token { Type: " + Lexer::Tokens::TypeToString(type) + ", Value: \"" + value + "\""
               + ", Pos: [" + std::to_string(start_pos) + ", " + std::to_string(end_pos)
               + ")"
               + ", Lexeme: \"" + std::string(lexeme) + "\""
               + " }" + '\n';
    }
};

inline bool operator==(const Token & lhs, const Token & rhs) {
    return lhs.type == rhs.type && lhs.value == rhs.value && lhs.start_pos == rhs.start_pos &&
           lhs.end_pos == rhs.end_pos;
}
};  // namespace Lexer::Tokens
#endif  // TOKEN_HPP
