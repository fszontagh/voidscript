#include "Lexer/Operators.hpp"

namespace Lexer {

const std::vector<std::string> OPERATOR_RELATIONAL = { "==", "!=", "<", ">", "<=", ">=" };
const std::vector<std::string> OPERATOR_INCREMENT  = { "++", "--" };
const std::vector<std::string> OPERATOR_ASSIGNMENT = { "=", "+=", "-=", "*=", "/=", "%=" };
const std::vector<std::string> OPERATOR_LOGICAL    = { "&&", "||" };

const std::vector<std::string> OPERATOR_ARITHMETIC = { "+", "-", "*", "/", "%", "!" };
const std::vector<std::string> PUNCTUATION         = { "(", ")", "{", "}", "[", "]", ",", ";", ":", "->", "." };

bool isUnaryOperator(const std::string & op) {
    return op == "+" || op == "-" || op == "!";
}

}  // namespace Lexer
