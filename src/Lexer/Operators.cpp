#include "Lexer/Operators.hpp"

namespace Lexer {

const std::vector<std::string> OPERATOR_RELATIONAL = { "==", "!=", "<", ">", "<=", ">=" };
const std::vector<std::string> OPERATOR_INCREMENT  = { "++", "--" };
const std::vector<std::string> OPERATOR_ASSIGNMENT = { "=", "+=", "-=", "*=", "/=", "%=" };
const std::vector<std::string> OPERATOR_LOGICAL    = { "&&", "||" };

const std::vector<std::string> OPERATOR_ARITHMETIC = { "+", "-", "*", "/", "%", "!" };
const std::vector<std::string> PUNCTUATION         = { "(", ")", "{", "}", "[", "]", ",", ";" };

bool contains(const std::vector<std::string> & vec, const std::string & value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

bool isUnaryOperator(const std::string & op) {
    return op == "+" || op == "-" || op == "!";
}

bool isBinaryOperator(const std::string & op) {
    return contains(OPERATOR_ARITHMETIC, op) || contains(OPERATOR_LOGICAL, op) ||
           contains(OPERATOR_RELATIONAL, op);
}

} // namespace Lexer
