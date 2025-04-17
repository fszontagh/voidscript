#ifndef LEXER_HPP
#define LEXER_HPP

#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseException.hpp"
#include "Token.hpp"

namespace Lexer {
class Lexer {
  public:
    Lexer();
    void                       addNamespaceInput(const std::string & ns, const std::string & input);
    void                       setKeyWords(const std::unordered_map<std::string, Tokens::Type> & new_keywords);
    std::vector<Tokens::Token> tokenizeNamespace(const std::string & ns);
    std::vector<Tokens::Token> getTokens(const std::string & ns) const;

    class Exception : public BaseException {
      public:
        using BaseException::BaseException;

        Exception(const std::string & msg) {
            rawMessage_ = msg;
            context_    = "";
        }

        std::string formatMessage() const override { return "[LEXER ERROR]: " + rawMessage_; }
    };


  private:
    std::unordered_map<std::string, std::string>                inputs_;
    std::unordered_map<std::string, std::vector<Tokens::Token>> tokens_;
    std::unordered_map<std::string, size_t>                     positions_;
    std::unordered_map<std::string, int>                        line_numbers_;
    std::unordered_map<std::string, int>                        column_numbers_;
    std::unordered_map<std::string, Tokens::Type>               keywords;
    std::string                                                 operators_;
    Tokens::Token                                               nextToken();

    const std::string & input() const;
    size_t &            pos();
    int &               line();
    int &               col();
    char                peek(size_t offset = 0) const;
    char                advance();
    bool                isAtEnd() const;
    void                skipWhitespaceAndComments();
    static bool         matchFromVector(const std::vector<std::string> & vec, const std::string & value);
    Tokens::Token       createToken(Tokens::Type type, size_t start, size_t end, const std::string & value = "");
    Tokens::Token       matchIdentifierOrKeyword(size_t start_pos, Tokens::Type type = Tokens::Type::IDENTIFIER);
    Tokens::Token       matchNumber(size_t start_pos);
    Tokens::Token       matchStringLiteral(size_t start_pos);
    Tokens::Token       matchOperatorOrPunctuation(size_t start_pos);

};  // class Lexer

};  // namespace Lexer
#endif  // LEXER_HPP
