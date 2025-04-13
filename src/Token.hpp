#ifndef TOKEN_HPP
#define TOKEN_HPP
#include <cstdint>
#include <string>
#include <unordered_map>

#include "VariableTypes.hpp"

enum class TokenType : std::uint8_t {
    ParserOpenTag,
    ParserCloseTag,
    ParserIfStatement,  // if
    FileClose,
    Identifier,
    StringLiteral,
    IntLiteral,
    DoubleLiteral,
    BooleanLiteral,
    LeftParenthesis,      // (
    RightParenthesis,     // )
    Comma,                // ,
    Semicolon,            // ;
    Variable,             // $variable
    VariableSign,         // $ variable start sign
    StringDeclaration,    // string $variable
    IntDeclaration,       // int $variable
    DoubleDeclaration,    // double $variable
    BooleanDeclaration,   // bool $variable
    FunctionDeclaration,  // function $variable
    Equals,               // =
    Plus,                 // +
    Minus,                // -
    Multiply,             // *
    Divide,               // /
    Modulo,               // %
    GreaterThan,          // >
    LessThan,             // <
    GreaterThanOrEqual,   // >=
    LessThanOrEqual,      // <=
    NotEqual,             // !=
    Equal,                // ==
    Not,                  // !
    And,                  // &&
    Or,                   // ||
    LeftBracket,          // [
    RightBracket,         // ]
    LeftCurlyBracket,     // {
    RightCurlyBracket,    // }
    EndOfFile,            // \0
    EndOfLine,            // \n
    Comment,              // #
    Unknown               // Unknown
};

const static std::unordered_map<TokenType, std::string> tokenTypeNames = {
    { TokenType::ParserOpenTag,       "ParserOpenTag"       },
    { TokenType::ParserCloseTag,      "ParserCloseTag"      },
    { TokenType::ParserIfStatement,   "ParserIfStatement"   },
    { TokenType::FileClose,           "FileClose"           },
    { TokenType::Identifier,          "Identifier"          },
    { TokenType::StringLiteral,       "StringLiteral"       },
    { TokenType::IntLiteral,          "IntLiteral"          },
    { TokenType::DoubleLiteral,       "DoubleLiteral"       },
    { TokenType::BooleanLiteral,      "BooleanLiteral"      },
    { TokenType::LeftParenthesis,     "LeftParenthesis"     },
    { TokenType::RightParenthesis,    "RightParenthesis"    },
    { TokenType::Comma,               "Comma"               },
    { TokenType::Semicolon,           "Semicolon"           },
    { TokenType::Variable,            "Variable"            },
    { TokenType::VariableSign,        "VariableSign"        },
    { TokenType::StringDeclaration,   "StringDeclaration"   },
    { TokenType::IntDeclaration,      "IntDeclaration"      },
    { TokenType::DoubleDeclaration,   "DoubleDeclaration"   },
    { TokenType::BooleanDeclaration,  "BooleanDeclaration"  },
    { TokenType::FunctionDeclaration, "FunctionDeclaration" },
    { TokenType::Equals,              "Equals"              },
    { TokenType::Plus,                "Plus"                },
    { TokenType::Minus,               "Minus"               },
    { TokenType::Multiply,            "Multiply"            },
    { TokenType::Divide,              "Divide"              },
    { TokenType::Modulo,              "Modulo"              },
    { TokenType::GreaterThan,         "GreaterThan"         },
    { TokenType::LessThan,            "LessThan"            },
    { TokenType::GreaterThanOrEqual,  "GreaterThanOrEqual"  },
    { TokenType::LessThanOrEqual,     "LessThanOrEqual"     },
    { TokenType::NotEqual,            "NotEqual"            },
    { TokenType::Equal,               "Equal"               },
    { TokenType::Not,                 "Not"                 },
    { TokenType::And,                 "And"                 },
    { TokenType::Or,                  "Or"                  },
    { TokenType::LeftBracket,         "LeftBracket"         },
    { TokenType::RightBracket,        "RightBracket"        },
    { TokenType::LeftCurlyBracket,    "LeftCurlyBracket"    },
    { TokenType::RightCurlyBracket,   "RightCurlyBracket"   },
    { TokenType::EndOfFile,           "EndOfFile"           },
    { TokenType::EndOfLine,           "EndOfLine"           },
    { TokenType::Comment,             "Comment"             },
    { TokenType::Unknown,             "Unknown"             }
};

[[nodiscard]] static inline std::string getTokenTypeAsString(TokenType type) {
    auto it = tokenTypeNames.find(type);
    if (it != tokenTypeNames.end()) {
        return it->second;
    }
    return "Unknown";
    //throw std::runtime_error("Unknown token type");
};

static const std::unordered_map<TokenType, Variables::Type> tokenTypeToVariableType = {
    { TokenType::StringLiteral,  Variables::Type::VT_STRING  },
    { TokenType::IntLiteral,     Variables::Type::VT_INT     },
    { TokenType::DoubleLiteral,  Variables::Type::VT_DOUBLE  },
    { TokenType::BooleanLiteral, Variables::Type::VT_BOOLEAN }
};

static const std::unordered_map<Variables::Type, TokenType> variableTypeToTokenType = {
    { Variables::Type::VT_STRING,  TokenType::StringLiteral  },
    { Variables::Type::VT_INT,     TokenType::IntLiteral     },
    { Variables::Type::VT_DOUBLE,  TokenType::DoubleLiteral  },
    { Variables::Type::VT_BOOLEAN, TokenType::BooleanLiteral }
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

[[nodiscard]] static inline TokenType getTokenTypeFromVariableType(Variables::Type type) {
    auto it = variableTypeToTokenType.find(type);
    if (it != variableTypeToTokenType.end()) {
        return it->second;
    }
    return TokenType::Unknown;
};

[[nodiscard]] static inline TokenType getTokenTypeFromValueDeclaration(const Variables::Type & declaration) {
    if (declaration == Variables::Type::VT_STRING) {
        return TokenType::StringDeclaration;
    }
    if (declaration == Variables::Type::VT_INT) {
        return TokenType::IntDeclaration;
    }
    if (declaration == Variables::Type::VT_DOUBLE) {
        return TokenType::DoubleDeclaration;
    }
    if (declaration == Variables::Type::VT_BOOLEAN) {
        return TokenType::BooleanDeclaration;
    }
    if (declaration == Variables::Type::VT_FUNCTION) {
        return TokenType::FunctionDeclaration;
    }
    return TokenType::Unknown;
}

[[nodiscard]] static inline Variables::Type getVariableTypeFromTokenTypeDeclaration(const TokenType & type) {
    if (type == TokenType::StringDeclaration) {
        return Variables::Type::VT_STRING;
    }
    if (type == TokenType::IntDeclaration) {
        return Variables::Type::VT_INT;
    }
    if (type == TokenType::DoubleDeclaration) {
        return Variables::Type::VT_DOUBLE;
    }
    if (type == TokenType::BooleanDeclaration) {
        return Variables::Type::VT_BOOLEAN;
    }
    if (type == TokenType::FunctionDeclaration) {
        return Variables::Type::VT_FUNCTION;
    }
    return Variables::Type::VT_NULL;
};

struct TokenPos {
    size_t start;
    size_t end;
};

struct Token {
    TokenType       type;
    std::string     lexeme;
    std::string     file;
    int             lineNumber;
    size_t          columnNumber;
    TokenPos        pos;
    Variables::Type variableType = Variables::Type::VT_NULL;

    [[nodiscard]] std::string getTypeName() const { return tokenTypeNames.at(type); }

    [[nodiscard]] std::string getVariableTypeName() const { return Variables::TypeToString(variableType); }
};

#endif  // TOKEN_HPP
