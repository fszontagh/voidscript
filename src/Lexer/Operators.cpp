#include "Lexer/Operators.hpp"

namespace Lexer {

const std::vector<std::string> OPERATOR_RELATIONAL = { "==", "!=", "<", ">", "<=", ">=" };
const std::vector<std::string> OPERATOR_INCREMENT  = { "++", "--" };
const std::vector<std::string> OPERATOR_ASSIGNMENT = { "=", "+=", "-=", "*=", "/=", "%=" };
const std::vector<std::string> OPERATOR_LOGICAL    = { "&&", "||" };
// Two-character bitwise shifts. Matched in the two-char pass, before '<' and '>'.
const std::vector<std::string> OPERATOR_BITWISE    = { "<<", ">>" };

// Single-character bitwise operators live here too; '&&' and '||' are matched by the
// two-char pass first, so a lone '&' or '|' reaching this table is genuinely bitwise.
const std::vector<std::string> OPERATOR_ARITHMETIC = { "+", "-", "*", "/", "%", "!", "&", "|", "^", "~" };
const std::vector<std::string> PUNCTUATION         = { "(", ")", "{", "}", "[", "]", ",", ";", ":", "->", "." };

bool isUnaryOperator(const std::string & op) {
    return op == "+" || op == "-" || op == "!" || op == "~";
}

}  // namespace Lexer
