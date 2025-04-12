#ifndef TOKEN_HPP
#define TOKEN_HPP
#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "VariableTypes.hpp"

enum class TokenType : std::uint8_t {
    ParserOpenTag,
    ParserCloseTag,
    FileClose,
    Identifier,
    StringLiteral,
    IntLiteral,
    DoubleLiteral,
    LeftParenthesis,
    RightParenthesis,
    Comma,
    Semicolon,
    Variable,           // $variable
    VariableSign,       // $ variable start sign
    StringDeclaration,  // string $variable
    IntDeclaration,     // int $variable
    DoubleDeclaration,  // double $variable
    Equals,             // = jel
    EndOfFile,
    EndOfLine,
    Comment,
    Unknown
};

const static std::unordered_map<TokenType, std::string> tokenTypeNames = {
    { TokenType::ParserOpenTag,     "ParserOpenTag"     },
    { TokenType::ParserCloseTag,    "ParserCloseTag"    },
    { TokenType::FileClose,         "FileClose"         },
    { TokenType::Identifier,        "Identifier"        },
    { TokenType::StringLiteral,     "StringLiteral"     },
    { TokenType::IntLiteral,        "IntLiteral"        },
    { TokenType::DoubleLiteral,     "DoubleLiteral"     },
    { TokenType::LeftParenthesis,   "LeftParenthesis"   },
    { TokenType::RightParenthesis,  "RightParenthesis"  },
    { TokenType::Comma,             "Comma"             },
    { TokenType::Semicolon,         "Semicolon"         },
    { TokenType::Variable,          "Variable"          },
    { TokenType::VariableSign,      "VariableSign"      },
    { TokenType::StringDeclaration, "StringDeclaration" },
    { TokenType::IntDeclaration,    "IntDeclaration"    },
    { TokenType::DoubleDeclaration, "DoubleDeclaration" },
    { TokenType::Equals,            "Equals"            },
    { TokenType::EndOfFile,         "EndOfFile"         },
    { TokenType::EndOfLine,         "EndOfLine"         },
    { TokenType::Comment,           "Comment"           },
    { TokenType::Unknown,           "Unknown"           }
};

static const std::unordered_map<TokenType, Variables::Type> tokenTypeToVariableType = {
    { TokenType::StringLiteral, Variables::Type::VT_STRING },
    { TokenType::IntLiteral,    Variables::Type::VT_INT    },
    { TokenType::DoubleLiteral, Variables::Type::VT_DOUBLE }
};

[[nodiscard]] static inline Variables::Type getVariableTypeFromTokenType(TokenType type) {
    auto it = tokenTypeToVariableType.find(type);
    if (it != tokenTypeToVariableType.end()) {
        return it->second;
    }

    return Variables::Type::VT_NOT_DEFINED;
}

[[nodiscard]] static inline std::string getVariableTypeFromTokenTypeAsString(TokenType type) {
    return Variables::TypeToString(getVariableTypeFromTokenType(type));
}

struct Token {
    TokenType       type;
    std::string     lexeme;
    std::string     file;
    int             lineNumber;
    size_t          columnNumber;
    Variables::Type variableType = Variables::Type::VT_NULL;

    [[nodiscard]] std::string getTypeName() const { return tokenTypeNames.at(type); }

    [[nodiscard]] std::string getVariableTypeName() const { return Variables::TypeToString(variableType); }
};

#endif  // TOKEN_HPP
