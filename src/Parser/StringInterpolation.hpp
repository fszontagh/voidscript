#ifndef PARSER_STRING_INTERPOLATION_HPP
#define PARSER_STRING_INTERPOLATION_HPP

#include <string>
#include <string_view>
#include <vector>

#include "Parser/ParsedExpression.hpp"

namespace Parser {

/**
 * @brief Splits a double-quoted string literal into a concatenation of literal text
 *        and the variables interpolated into it.
 *
 * Works from the token's raw lexeme rather than its processed value, because by the
 * time the lexer has resolved escapes an escaped `\$` is indistinguishable from a real
 * one, and the distinction is exactly what decides whether to interpolate.
 *
 * Supported forms:
 *   "$name"           - bare identifier
 *   "${name}"         - braced, so the name can be delimited: "${name}s"
 *   "${obj->field}"   - braced member access, any depth
 *   "\$"              - escaped, stays a literal dollar
 *
 * A `$` not followed by an identifier character or `{` is left alone, so "100% $ ok"
 * needs no escaping.
 */
namespace interpolation {

inline bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

inline bool isIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

/** Resolve backslash escapes in a literal segment, matching the lexer's own table. */
inline std::string unescape(std::string_view raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] != '\\' || i + 1 >= raw.size()) {
            out += raw[i];
            continue;
        }
        switch (raw[++i]) {
            case 'n':  out += '\n'; break;
            case 't':  out += '\t'; break;
            case 'r':  out += '\r'; break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'v':  out += '\v'; break;
            case 'a':  out += '\a'; break;
            case '0':  out += '\0'; break;
            default:   out += raw[i]; break;  // covers \" \' \\ and \$
        }
    }
    return out;
}

/** Build `$name` or `$obj->field->deeper` as a ParsedExpression. */
inline ParsedExpressionPtr buildReference(const std::string & ref, const std::string & filename, int line,
                                          size_t column) {
    size_t arrow = ref.find("->");
    if (arrow == std::string::npos) {
        return ParsedExpression::makeVariable(ref, filename, line, column);
    }

    auto   expr = ParsedExpression::makeVariable(ref.substr(0, arrow), filename, line, column);
    size_t pos  = arrow;
    while (pos != std::string::npos) {
        size_t start = pos + 2;
        size_t next  = ref.find("->", start);
        std::string member = ref.substr(start, next == std::string::npos ? std::string::npos : next - start);
        expr = ParsedExpression::makeBinary("->", std::move(expr),
                                            ParsedExpression::makeLiteral(Symbols::ValuePtr(member)), filename, line,
                                            column);
        pos = next;
    }
    return expr;
}

/**
 * @brief Returns the interpolated expression, or nullptr if the literal needs no
 *        interpolation and the caller should use the token's plain value.
 *
 * @param rawWithQuotes the token lexeme, quotes included.
 */
inline ParsedExpressionPtr interpolate(std::string_view rawWithQuotes, const std::string & filename, int line,
                                       size_t column) {
    // Only double-quoted literals interpolate; single quotes are the opt-out.
    if (rawWithQuotes.size() < 2 || rawWithQuotes.front() != '"') {
        return nullptr;
    }
    std::string_view body = rawWithQuotes.substr(1, rawWithQuotes.size() - 2);

    std::vector<ParsedExpressionPtr> parts;
    std::string                      literal;
    bool                             sawInterpolation = false;

    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) {
            literal += body[i];
            literal += body[i + 1];
            ++i;
            continue;
        }
        if (body[i] != '$' || i + 1 >= body.size()) {
            literal += body[i];
            continue;
        }

        std::string ref;
        size_t      after = 0;
        if (body[i + 1] == '{') {
            size_t close = body.find('}', i + 2);
            if (close == std::string_view::npos) {
                literal += body[i];
                continue;
            }
            ref   = std::string(body.substr(i + 2, close - (i + 2)));
            after = close + 1;
        } else if (isIdentStart(body[i + 1])) {
            size_t end = i + 1;
            while (end < body.size() && isIdentChar(body[end])) {
                ++end;
            }
            ref   = std::string(body.substr(i + 1, end - (i + 1)));
            after = end;
        } else {
            // A lone '$', or one before punctuation: not an interpolation.
            literal += body[i];
            continue;
        }

        if (ref.empty()) {
            literal += body[i];
            continue;
        }

        if (!literal.empty()) {
            parts.push_back(ParsedExpression::makeLiteral(Symbols::ValuePtr(unescape(literal))));
            literal.clear();
        }
        parts.push_back(buildReference(ref, filename, line, column));
        sawInterpolation = true;
        i                = after - 1;
    }

    if (!sawInterpolation) {
        return nullptr;
    }
    if (!literal.empty()) {
        parts.push_back(ParsedExpression::makeLiteral(Symbols::ValuePtr(unescape(literal))));
    }

    // Start from an empty string so a literal that is nothing but a reference still
    // produces a string, e.g. "$n" with an int $n yields "42" rather than 42.
    auto expr = ParsedExpression::makeLiteral(Symbols::ValuePtr(std::string{}));
    for (auto & part : parts) {
        expr = ParsedExpression::makeBinary("+", std::move(expr), std::move(part), filename, line, column);
    }
    return expr;
}

}  // namespace interpolation
}  // namespace Parser

#endif  // PARSER_STRING_INTERPOLATION_HPP
