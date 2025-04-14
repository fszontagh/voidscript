#ifndef LEXER_HPP
#define LEXER_HPP

#include <algorithm>
#include <istream>
#include <sstream>
#include <vector>

#include "StringHelpers.hpp"
#include "Token.hpp"
#include "VariableTypes.hpp"

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
    Token boolean();
    Token singleCharToken(TokenType type, const std::string & lexeme);
    bool  matchSequence(const std::string & sequence, bool caseSensitive = true) const;
    Token variableDeclaration(Variables::Type type);
    void  matchAndConsume(const std::string & sequence, bool caseSensitive = true);

    // create token methods
    Token createToken(TokenType type, const std::string & lexeme) const;
    Token createSingleCharToken(TokenType type, const std::string & lexeme);
    Token createUnknownToken(const std::string & lexeme) const;
    Token createErrorToken(const std::string & lexeme) const;
    Token stringToken();
    Token numberToken();
    Token identifierToken();
    Token variableToken();
    Token commentToken();
    Token keywordOrIdentifierToken();
    Token functionDeclarationToken();
    Token variableDeclarationToken(Variables::Type type);

    // validate number types from string
    template <typename Numeric> static bool is_number(const std::string & s) {
        Numeric n;
        return ((std::istringstream(s) >> n >> std::ws).eof());
    }

    bool matchSequence(const std::string & sequence, bool caseSensitive = true) {
        if (caseSensitive) {
            return src.substr(pos, sequence.length()) == sequence;
        }

        std::string srcSubstr = src.substr(pos, sequence.length());
        std::string seqLower  = sequence;

        StringHelpers::strtolower(srcSubstr);
        StringHelpers::strtolower(seqLower);

        return srcSubstr == seqLower;
    }
};

#endif  // LEXER_HPP
