#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>
#include <sstream>
#include <vector>

#include "VariableTypes.hpp"
#include "options.h"
#include "Token.hpp"

class Lexer {
  public:
    Lexer(const std::string & source, const std::string & filename);
    std::vector<Token> tokenize();

  private:
    const std::string & src;
    const std::string & filename;
    size_t              pos;
    int                 lineNumber = 1;
    size_t              colNumber  = 1;
    size_t              charNumber = 1;

    char peek() const;
    char advance();
    bool isAtEnd() const;

    Token string();
    Token number();
    Token identifier();
    Token variable();
    Token comment();
    Token keywordOrIdentifier();
    Token singleCharToken(TokenType type, const std::string & lexeme);
    bool matchSequence(const std::string & sequence) const;
    Token variableDeclaration(Variables::Type type);
    void matchAndConsume(const std::string & sequence);


    // validate number types from string
    template <typename Numeric> static bool is_number(const std::string & s) {
        Numeric n;
        return ((std::istringstream(s) >> n >> std::ws).eof());
    }

    bool matchSequence(const std::string & sequence) { return src.substr(pos, sequence.length()) == sequence; }
};

#endif  // LEXER_HPP
