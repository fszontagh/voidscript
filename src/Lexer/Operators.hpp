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
bool isBinaryOperator(const std::string & op);

inline int getPrecedence(const std::string & op) {
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
    if (op.starts_with("u")) {
        std::string real_op = op.substr(1);  // "u!" -> "!"
        return Parser::ParsedExpression::makeUnary(real_op, std::move(rhs));
    }
    return Parser::ParsedExpression::makeBinary(op, std::move(lhs), std::move(rhs));
}

[[nodiscard]] inline bool pushOperand(const Tokens::Token & token, const Symbols::Variables::Type & expected_var_type,
                                      std::vector<Parser::ParsedExpressionPtr> & output_queue) {
    if (token.type == Tokens::Type::NUMBER || token.type == Tokens::Type::STRING_LITERAL ||
        token.type == Tokens::Type::KEYWORD) {
        // Parse literal: use expected type if provided, otherwise auto-detect
        if (expected_var_type == Symbols::Variables::Type::NULL_TYPE) {
            output_queue.push_back(
                Parser::ParsedExpression::makeLiteral(
                    Symbols::Value::fromString(token.value, /*autoDetectType*/ true)));
        } else {
            output_queue.push_back(
                Parser::ParsedExpression::makeLiteral(
                    Symbols::Value::fromString(token.value, expected_var_type)));
        }
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
