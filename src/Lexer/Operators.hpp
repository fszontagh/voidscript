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
extern const std::vector<std::string> OPERATOR_BITWISE;

// one char
extern const std::vector<std::string> OPERATOR_ARITHMETIC;
extern const std::vector<std::string> PUNCTUATION;

bool contains(const std::vector<std::string> & vec, const std::string & value);
bool isUnaryOperator(const std::string & op);


// Binding strength, loosest first, following C. Only the relative order matters.
// Note '[]' must be listed: when it was absent it fell through to the -1 default and
// bound looser than every operator, so $q->outputs[0] grouped as $q->(outputs[0]).
inline int getPrecedence(const std::string & op) {
    if (op == "||") {
        return 0;
    }
    if (op == "&&") {
        return 1;
    }
    if (op == "|") {
        return 2;
    }
    if (op == "^") {
        return 3;
    }
    if (op == "&") {
        return 4;
    }
    if (op == "==" || op == "!=") {
        return 5;
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        return 6;
    }
    if (op == "<<" || op == ">>") {
        return 7;
    }
    if (op == "+" || op == "-") {
        return 8;
    }
    if (op == "*" || op == "/" || op == "%") {
        return 9;
    }
    if (op == "u-" || op == "u+" || op == "u!" || op == "u~") {
        return 10;
    }
    // Accessors bind tightest: '->', '::' and indexing, all left-associative.
    if (op == "->" || op == "::" || op == "[]") {
        return 11;
    }
    return -1;
}

inline bool isLeftAssociative(const std::string & op) {
    return !(op == "u-" || op == "u+" || op == "u~" || op == "u!");
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
                                      std::vector<Parser::ParsedExpressionPtr> & output_queue, const std::string & filename = "") {
    // Literal operands: number, string, or keyword literals (e.g., true/false/null)
    // Literals are NOT gated on expected_var_type. A declaration's type says what the
    // whole expression must produce, not what may appear inside it: `string $s = "x" + 5;`
    // and `string $s = $c ? "a" : "b";` are both legitimate, and gating here rejected
    // them at parse time. DeclareVariableStatementNode still type checks the final
    // value, and its message ("expected 'string' but got 'int'") is clearer than the
    // parse error this replaced.
    if (token.type == Tokens::Type::NUMBER) {
        // Auto-detect or cast to appropriate numeric type
        output_queue.push_back(Parser::ParsedExpression::makeLiteral(Symbols::ValuePtr::fromString(token.value)));
        return true;
    }
    if (token.type == Tokens::Type::STRING_LITERAL) {
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
        // Use location information from the token
        output_queue.push_back(Parser::ParsedExpression::makeVariable(name, filename, token.line_number, token.column_number));
        return true;
    }
    return false;
}

};  // namespace Lexer

#endif  // LEXER_OPERATORS_HPP
