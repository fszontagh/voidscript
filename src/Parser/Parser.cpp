#include "Parser/Parser.hpp"

// Más szükséges include-ok, ha kellenek
namespace Parser {

const std::unordered_map<std::string, Lexer::Tokens::Type> Parser::keywords = {
    { "if",       Lexer::Tokens::Type::KEYWORD          },
    { "else",     Lexer::Tokens::Type::KEYWORD          },
    { "while",    Lexer::Tokens::Type::KEYWORD          },
    { "for",      Lexer::Tokens::Type::KEYWORD          },
    { "return",   Lexer::Tokens::Type::KEYWORD_RETURN   },
    { "function", Lexer::Tokens::Type::KEYWORD_FUNCTION },
    // Régebbiek:
    { "const",    Lexer::Tokens::Type::KEYWORD          },
    { "true",     Lexer::Tokens::Type::KEYWORD          },
    { "false",    Lexer::Tokens::Type::KEYWORD          },
    // változó típusok
    { "null",     Lexer::Tokens::Type::KEYWORD_NULL     },
    { "int",      Lexer::Tokens::Type::KEYWORD_INT      },
    { "double",   Lexer::Tokens::Type::KEYWORD_DOUBLE   },
    { "float",    Lexer::Tokens::Type::KEYWORD_FLOAT    },
    { "string",   Lexer::Tokens::Type::KEYWORD_STRING   },
    { "boolean",  Lexer::Tokens::Type::KEYWORD_BOOLEAN  },
    { "bool",     Lexer::Tokens::Type::KEYWORD_BOOLEAN  },
    // ... egyéb kulcsszavak ...
};

const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> Parser::variable_types = {
    { Lexer::Tokens::Type::KEYWORD_INT,     Symbols::Variables::Type::INTEGER   },
    { Lexer::Tokens::Type::KEYWORD_DOUBLE,  Symbols::Variables::Type::DOUBLE    },
    { Lexer::Tokens::Type::KEYWORD_FLOAT,   Symbols::Variables::Type::FLOAT     },
    { Lexer::Tokens::Type::KEYWORD_STRING,  Symbols::Variables::Type::STRING    },
    { Lexer::Tokens::Type::KEYWORD_NULL,    Symbols::Variables::Type::NULL_TYPE },
    { Lexer::Tokens::Type::KEYWORD_BOOLEAN, Symbols::Variables::Type::BOOLEAN   },
};

}  // namespace Parser
