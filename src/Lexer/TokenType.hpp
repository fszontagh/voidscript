#ifndef TOKEN_TYPE_HPP
#define TOKEN_TYPE_HPP

#include <cstdint>
#include <string>

namespace Lexer::Tokens {

enum class Type : std::uint8_t {
    IDENTIFIER,           //Eg.variable name: myVar
    VARIABLE_IDENTIFIER,  //Eg.$
    KEYWORD,              //Eg.if, else, while
    NUMBER,               //Eg.123, 42
    STRING_LITERAL,       //Eg."hello world"
    OPERATOR_RELATIONAL,  //Eg.+, -, *, /, =, ==, <, >
    OPERATOR_LOGICAL,     //Eg.&&, ||
    OPERATOR_ASSIGNMENT,  //Eg.+=, -=, *=, /=
    OPERATOR_INCREMENT,   //Eg.++, --
    OPERATOR_ARITHMETIC,  //Eg.+, -, *, /
    OPERATOR_NAMESPACE_RESOLUTION, // ADDED FOR ::
    PUNCTUATION,          //Eg.(, ), {, }, ;, ,
    COMMENT,              // Just skip it
    END_OF_FILE,          // End of the file / script
    KEYWORD_STRING,
    KEYWORD_INT,
    KEYWORD_DOUBLE,
    KEYWORD_FLOAT,
    KEYWORD_BOOLEAN,
    KEYWORD_OBJECT,
    KEYWORD_FUNCTION_DECLARATION,
    KEYWORD_RETURN,
    KEYWORD_NULL,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_WHILE,
    KEYWORD_CONST,
    KEYWORD_INCLUDE,
    // Class-related keywords
    KEYWORD_CLASS,
    KEYWORD_PRIVATE,
    KEYWORD_PUBLIC,
    KEYWORD_NEW,
    KEYWORD_THIS,
    // Enum and Switch keywords
    KEYWORD_ENUM,
    KEYWORD_SWITCH,
    KEYWORD_CASE,
    KEYWORD_DEFAULT,
    KEYWORD_BREAK, // Added for break statements
    UNKNOWN  // Unknown token type
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
        case Lexer::Tokens::Type::OPERATOR_NAMESPACE_RESOLUTION: // ADDED
            return "OPERATOR_NAMESPACE_RESOLUTION";             // ADDED
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
        case Lexer::Tokens::Type::KEYWORD_OBJECT:
            return "KEYWORD_OBJECT";
        case Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION:
            return "KEYWORD_FUNCTION";
        case Lexer::Tokens::Type::KEYWORD_RETURN:
            return "KEYWORD_RETURN";
        case Lexer::Tokens::Type::KEYWORD_NULL:
            return "KEYWORD_NULL";
        case Lexer::Tokens::Type::KEYWORD_CLASS:
            return "KEYWORD_CLASS";
        case Lexer::Tokens::Type::KEYWORD_PRIVATE:
            return "KEYWORD_PRIVATE";
        case Lexer::Tokens::Type::KEYWORD_PUBLIC:
            return "KEYWORD_PUBLIC";
        case Lexer::Tokens::Type::KEYWORD_NEW:
            return "KEYWORD_NEW";
        case Lexer::Tokens::Type::KEYWORD_THIS:
            return "KEYWORD_THIS";
        case Lexer::Tokens::Type::KEYWORD_WHILE:
            return "KEYWORD_WHILE";
        case Lexer::Tokens::Type::KEYWORD_IF:
            return "KEYWORD_IF";
        case Lexer::Tokens::Type::KEYWORD_ELSE:
            return "KEYWORD_ELSE";
        case Lexer::Tokens::Type::KEYWORD_FOR:
            return "KEYWORD_FOR";
        case Lexer::Tokens::Type::KEYWORD_CONST:
            return "KEYWORD_CONST";
        case Lexer::Tokens::Type::KEYWORD_INCLUDE:
            return "KEYWORD_INCLUDE";
        case Lexer::Tokens::Type::KEYWORD_ENUM:
            return "KEYWORD_ENUM";
        case Lexer::Tokens::Type::KEYWORD_SWITCH:
            return "KEYWORD_SWITCH";
        case Lexer::Tokens::Type::KEYWORD_CASE:
            return "KEYWORD_CASE";
        case Lexer::Tokens::Type::KEYWORD_DEFAULT:
            return "KEYWORD_DEFAULT";
        case Lexer::Tokens::Type::KEYWORD_BREAK:
            return "KEYWORD_BREAK";
        case Lexer::Tokens::Type::UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID_TYPE";
    }
}

};  // namespace Lexer::Tokens

#endif  // TOKEN_TYPE_HPP
