#ifndef LEXER_OPERATORS_HPP
#define LEXER_OPERATORS_HPP

#include <string>
#include <vector>

#include "Lexer/Token.hpp"
#include "Parser/ParsedExpression.hpp"

namespace Lexer {

// two chars
extern const std::vector<std::string> OPERATOR_RELATIONAL;
extern const std::vector<std::string> OPERATOR_INCREMENT;
extern const std::vector<std::string> OPERATOR_ASSIGNMENT;
extern const std::vector<std::string> OPERATOR_LOGICAL;

// one char
extern const std::vector<std::string> OPERATOR_ARITHMETIC;
extern const std::vector<std::string> PUNCTUATION;

bool contains(const std::vector<std::string> & vec, const std::string & value);
bool isUnaryOperator(const std::string & op);


inline int getPrecedence(const std::string & op) {
    if (op == "->") {
        return 5;  // Member access has highest precedence
    }
    if (op == "u-" || op == "u+" || op == "u!") {
        return 4;
    }
    if (op == "*" || op == "/" || op == "%") {
        return 3;
    }
    if (op == "+" || op == "-") {
        return 2;
    }
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
        return 1;
    }
    if (op == "&&" || op == "||") {
        return 0;
    }
    return -1;
}

inline bool isLeftAssociative(const std::string & op) {
    return !(op == "u-" || op == "u+");
}

inline Parser::ParsedExpressionPtr applyOperator(const std::string & op, Parser::ParsedExpressionPtr rhs,
                                                 Parser::ParsedExpressionPtr lhs = nullptr) {
    if (op.starts_with("u") || op.starts_with("p")) {
        std::string real_op = op.substr(1);  // "u!" -> "!"
        auto        result =
            Parser::ParsedExpression::makeUnary(real_op, std::move(rhs), rhs->filename, rhs->line, rhs->column);
        // Copy source location from operand if available

        return result;
    }
    auto result =
        Parser::ParsedExpression::makeBinary(op, std::move(lhs), std::move(rhs), lhs->filename, lhs->line, lhs->column);

    return result;
}

[[nodiscard]] inline bool pushOperand(const Tokens::Token & token, const Symbols::Variables::Type & expected_var_type,
                                      std::vector<Parser::ParsedExpressionPtr> & output_queue) {
    // Literal operands: number, string, or keyword literals (e.g., true/false/null)
    if (token.type == Tokens::Type::NUMBER) {
        // Numeric literal: only allowed if expected is numeric or unspecified
        if (expected_var_type != Symbols::Variables::Type::NULL_TYPE &&
            expected_var_type != Symbols::Variables::Type::INTEGER &&
            expected_var_type != Symbols::Variables::Type::DOUBLE &&
            expected_var_type != Symbols::Variables::Type::FLOAT) {
            return false;
        }
        // Auto-detect or cast to expected numeric type
        output_queue.push_back(Parser::ParsedExpression::makeLiteral(Symbols::ValuePtr::fromString(token.value)));
        return true;
    }
    if (token.type == Tokens::Type::STRING_LITERAL) {
        // String literal: only allowed if expected is string or unspecified
        if (expected_var_type != Symbols::Variables::Type::NULL_TYPE &&
            expected_var_type != Symbols::Variables::Type::STRING) {
            return false;
        }
        output_queue.push_back(Parser::ParsedExpression::makeLiteral(token.value));
        return true;
    }
    if (token.type == Tokens::Type::KEYWORD) {
        // Keyword literal: e.g., true, false, null
        Symbols::ValuePtr val   = Symbols::ValuePtr::fromString(token.value);
        // only allowed if expected matches or unspecified
        if (expected_var_type != Symbols::Variables::Type::NULL_TYPE && expected_var_type != val) {
            return false;
        }
        output_queue.push_back(Parser::ParsedExpression::makeLiteral(val));
        return true;
    }
    if (token.type == Tokens::Type::VARIABLE_IDENTIFIER) {
        std::string name = token.value;
        if (!name.empty() && name[0] == '$') {
            name = name.substr(1);
        }
        output_queue.push_back(Parser::ParsedExpression::makeVariable(name));
        return true;
    }
    return false;
}

};  // namespace Lexer

#endif  // LEXER_OPERATORS_HPP
