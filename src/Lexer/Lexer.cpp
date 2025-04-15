#include "Lexer/Lexer.hpp"

namespace Lexer {
const std::vector<std::string> Lexer::Lexer::OPERATOR_RELATIONAL = { "==", "!=", "<", ">", "<=", ">=" };
const std::vector<std::string> Lexer::Lexer::OPERATOR_INCREMENT  = { "++", "--" };
const std::vector<std::string> Lexer::Lexer::OPERATOR_ASSIGNMENT = { "=", "+=", "-=", "*=", "/=", "%=" };
const std::vector<std::string> Lexer::Lexer::OPERATOR_LOGICAL    = { "&&", "||" };

const std::vector<std::string> Lexer::Lexer::OPERATOR_ARITHMETIC = { "+", "-", "*", "/", "%" };
const std::vector<std::string> Lexer::Lexer::PUNCTUATION         = { "(", ")", "{", "}", "[", "]", ",", ";" };



};  // namespace Lexer
