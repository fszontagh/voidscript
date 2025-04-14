#ifndef TOKEN_HPP
#define TOKEN_HPP
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Tokens {
enum class Type : std::uint8_t {
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
    FunctionDeclaration,  // function fn_name
    FunctionCall,         // fn_name(args)
    Return,               // return
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

const static std::unordered_map<Tokens::Type, std::string> tokenTypeNames = {
    { Tokens::Type::ParserOpenTag,       "ParserOpenTag"       },
    { Tokens::Type::ParserCloseTag,      "ParserCloseTag"      },
    { Tokens::Type::ParserIfStatement,   "ParserIfStatement"   },
    { Tokens::Type::FileClose,           "FileClose"           },
    { Tokens::Type::Identifier,          "Identifier"          },
    { Tokens::Type::StringLiteral,       "StringLiteral"       },
    { Tokens::Type::IntLiteral,          "IntLiteral"          },
    { Tokens::Type::DoubleLiteral,       "DoubleLiteral"       },
    { Tokens::Type::BooleanLiteral,      "BooleanLiteral"      },
    { Tokens::Type::LeftParenthesis,     "LeftParenthesis"     },
    { Tokens::Type::RightParenthesis,    "RightParenthesis"    },
    { Tokens::Type::Comma,               "Comma"               },
    { Tokens::Type::Semicolon,           "Semicolon"           },
    { Tokens::Type::Variable,            "Variable"            },
    { Tokens::Type::VariableSign,        "VariableSign"        },
    { Tokens::Type::StringDeclaration,   "StringDeclaration"   },
    { Tokens::Type::IntDeclaration,      "IntDeclaration"      },
    { Tokens::Type::DoubleDeclaration,   "DoubleDeclaration"   },
    { Tokens::Type::BooleanDeclaration,  "BooleanDeclaration"  },
    { Tokens::Type::FunctionDeclaration, "FunctionDeclaration" },
    { Tokens::Type::FunctionCall,        "FunctionCall"        },
    { Tokens::Type::Return,              "Return"              },
    { Tokens::Type::Equals,              "Equals"              },
    { Tokens::Type::Plus,                "Plus"                },
    { Tokens::Type::Minus,               "Minus"               },
    { Tokens::Type::Multiply,            "Multiply"            },
    { Tokens::Type::Divide,              "Divide"              },
    { Tokens::Type::Modulo,              "Modulo"              },
    { Tokens::Type::GreaterThan,         "GreaterThan"         },
    { Tokens::Type::LessThan,            "LessThan"            },
    { Tokens::Type::GreaterThanOrEqual,  "GreaterThanOrEqual"  },
    { Tokens::Type::LessThanOrEqual,     "LessThanOrEqual"     },
    { Tokens::Type::NotEqual,            "NotEqual"            },
    { Tokens::Type::Equal,               "Equal"               },
    { Tokens::Type::Not,                 "Not"                 },
    { Tokens::Type::And,                 "And"                 },
    { Tokens::Type::Or,                  "Or"                  },
    { Tokens::Type::LeftBracket,         "LeftBracket"         },
    { Tokens::Type::RightBracket,        "RightBracket"        },
    { Tokens::Type::LeftCurlyBracket,    "LeftCurlyBracket"    },
    { Tokens::Type::RightCurlyBracket,   "RightCurlyBracket"   },
    { Tokens::Type::EndOfFile,           "EndOfFile"           },
    { Tokens::Type::EndOfLine,           "EndOfLine"           },
    { Tokens::Type::Comment,             "Comment"             },
    { Tokens::Type::Unknown,             "Unknown"             }
};

struct TokenPos {
    size_t start;
    size_t end;
};

struct Token {
    Tokens::Type type;
    std::string  lexeme;
    std::string  file;
    int          lineNumber;
    size_t       columnNumber;
    TokenPos     pos;

    [[nodiscard]] std::string getTokenName() const { return tokenTypeNames.at(type); }
};
}  // namespace Tokens
#endif  // TOKEN_HPP
