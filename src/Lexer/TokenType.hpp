#ifndef TOKEN_TYPE_HPP
#define TOKEN_TYPE_HPP

#include <cstdint>
#include <string>

namespace Lexer::Tokens {

enum class Type : std::uint8_t {
    IDENTIFIER,           // Pl. változónév: myVar
    VARIABLE_IDENTIFIER,  // Pl. $
    KEYWORD,              // Pl. if, else, while
    NUMBER,               // Pl. 123, 42
    STRING_LITERAL,       // Pl. "hello world"
    OPERATOR_RELATIONAL,  // Pl. +, -, *, /, =, ==, <, >
    OPERATOR_LOGICAL,     // Pl. &&, ||
    OPERATOR_ASSIGNMENT,  // Pl. +=, -=, *=, /=
    OPERATOR_INCREMENT,   // Pl. ++, --
    OPERATOR_ARITHMETIC,  // Pl. +, -, *, /
    PUNCTUATION,          // Pl. (, ), {, }, ;, ,
    COMMENT,              // Általában kihagyjuk
    END_OF_FILE,          // A string végét jelzi
    KEYWORD_STRING,
    KEYWORD_INT,
    KEYWORD_DOUBLE,
    KEYWORD_FLOAT,
    KEYWORD_BOOLEAN,
    KEYWORD_FUNCTION_DECLARATION,
    KEYWORD_RETURN,
    KEYWORD_NULL,
    UNKNOWN  // Ismeretlen karakter/szekvencia
};

inline std::string TypeToString(Lexer::Tokens::Type type) {
    switch (type) {
        case Lexer::Tokens::Type::IDENTIFIER:
            return "IDENTIFIER";
        case Lexer::Tokens::Type::VARIABLE_IDENTIFIER:
            return "VARIABLE_IDENTIFIER";
        case Lexer::Tokens::Type::KEYWORD:
            return "KEYWORD";
        case Lexer::Tokens::Type::NUMBER:
            return "NUMBER";
        case Lexer::Tokens::Type::STRING_LITERAL:
            return "STRING_LITERAL";
        case Lexer::Tokens::Type::OPERATOR_RELATIONAL:
            return "OPERATOR_RELATIONAL";
        case Lexer::Tokens::Type::OPERATOR_LOGICAL:
            return "OPERATOR_LOGICAL";
        case Lexer::Tokens::Type::OPERATOR_ASSIGNMENT:
            return "OPERATOR_ASSIGNMENT";
        case Lexer::Tokens::Type::OPERATOR_INCREMENT:
            return "OPERATOR_INCREMENT";
        case Lexer::Tokens::Type::OPERATOR_ARITHMETIC:
            return "OPERATOR_ARITHMETIC";
        case Lexer::Tokens::Type::PUNCTUATION:
            return "PUNCTUATION";
        case Lexer::Tokens::Type::COMMENT:
            return "COMMENT";
        case Lexer::Tokens::Type::END_OF_FILE:
            return "END_OF_FILE";
        case Lexer::Tokens::Type::KEYWORD_STRING:
            return "KEYWORD_STRING";
        case Lexer::Tokens::Type::KEYWORD_INT:
            return "KEYWORD_INT";
        case Lexer::Tokens::Type::KEYWORD_DOUBLE:
            return "KEYWORD_DOUBLE";
        case Lexer::Tokens::Type::KEYWORD_FLOAT:
            return "KEYWORD_FLOAT";
        case Lexer::Tokens::Type::KEYWORD_BOOLEAN:
            return "KEYWORD_BOOLEAN";
        case Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION:
            return "KEYWORD_FUNCTION";
        case Lexer::Tokens::Type::KEYWORD_RETURN:
            return "KEYWORD_RETURN";
        case Lexer::Tokens::Type::KEYWORD_NULL:
            return "KEYWORD_NULL";
        case Lexer::Tokens::Type::UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID_TYPE";
    }
}

};  // namespace Lexer::Tokens

#endif  // TOKEN_TYPE_HPP
