#include "Lexer.hpp"

#include <cctype>

#include "Value.hpp"

Lexer::Lexer(const std::string & source, const std::string & filename) :
    src(source),
    pos(0),
    filename(filename),
    lineNumber(1),
    colNumber(1),
    charNumber(0) {}

char Lexer::peek() const {
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::advance() {
    if (pos >= src.size()) {
        return '\0';
    }
    char c = src[pos++];
    if (c == '\n') {
        this->lineNumber++;
        this->colNumber = 1;
    } else {
        this->colNumber++;
    }
    this->charNumber++;
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= src.size();
}

Token Lexer::string() {
    std::string result;
    size_t      startCol = colNumber;
    advance();  // Skip opening quote
    while (!isAtEnd() && peek() != '"') {
        result += advance();
    }
    if (isAtEnd() || peek() != '"') {
        return { TokenType::Unknown, "Unterminated string", filename, lineNumber, startCol };
    }
    advance();  // Skip closing quote
    return { TokenType::StringLiteral, result, filename, lineNumber, startCol };
}

Token Lexer::number() {
    std::string result;
    std::string found;
    TokenType   type             = TokenType::Unknown;
    bool        decimalPointSeen = false;
    size_t      startCol         = colNumber;

    while (std::isdigit(peek()) || peek() == '.') {
        if (peek() == '.') {
            if (decimalPointSeen) {
                return { TokenType::Unknown, "Invalid number format", filename, lineNumber, startCol };
            }
            decimalPointSeen = true;
        }
        found.append(1, advance());
    }

    if (!found.empty()) {
        if (found.find('.') == std::string::npos) {
            if (is_number<int>(found)) {
                result = found;
                type   = TokenType::IntLiteral;
            } else {
                return { TokenType::Unknown, "Invalid integer", filename, lineNumber, startCol };
            }
        } else {
            if (is_number<double>(found)) {
                result = found;
                type   = TokenType::DoubleLiteral;
            } else {
                return { TokenType::Unknown, "Invalid double", filename, lineNumber, startCol };
            }
        }
    } else {
        return { TokenType::Unknown, "Expected number", filename, lineNumber, startCol };
    }

    return { type, result, filename, lineNumber, startCol };
}

Token Lexer::identifier() {
    std::string result;
    size_t      startCol = colNumber;
    while (isalnum(peek()) || peek() == '_') {
        result += advance();
    }
    return { TokenType::Identifier, result, filename, lineNumber, startCol };
}

Token Lexer::variable() {
    size_t startCol = colNumber;
    advance();  // Skip $
    std::string varName;
    if (isalpha(peek()) || peek() == '_') {
        varName += advance();
        while (isalnum(peek()) || peek() == '_') {
            varName += advance();
        }
        return { TokenType::Variable, varName, filename, lineNumber, startCol };
    }
    return { TokenType::Unknown, "$ followed by invalid character", filename, lineNumber, startCol };
}

Token Lexer::comment() {
    size_t startCol = colNumber;
    advance();  // Skip #
    std::string commentText;
    while (!isAtEnd() && peek() != '\n') {
        commentText += advance();
    }
    return { TokenType::Comment, commentText, filename, lineNumber, startCol };
}

Token Lexer::keywordOrIdentifier() {
    std::string lexeme;
    size_t      startCol = colNumber;
    while (isalpha(peek())) {
        lexeme += advance();
    }

    if (Variables::StringToTypeMap.contains(lexeme)) {
        const auto type = Variables::StringToTypeMap.at(lexeme);
        while (isspace(peek())) {
            advance();
        }
        if (peek() == '$') {
            return variableDeclaration(type);
        }
        return { TokenType::Identifier, lexeme, filename, lineNumber, startCol };
    }
    return { TokenType::Identifier, lexeme, filename, lineNumber, startCol };
}

Token Lexer::variableDeclaration(Variables::Type type) {
    size_t startCol = colNumber;
    advance();  // Skip $
    std::string varName;
    if (isalpha(peek()) || peek() == '_') {
        varName += advance();
        while (isalnum(peek()) || peek() == '_') {
            varName += advance();
        }
        switch (type) {
            case Variables::Type::VT_INT:
                return { TokenType::IntDeclaration, varName, filename, lineNumber, startCol };
            case Variables::Type::VT_DOUBLE:
                return { TokenType::DoubleDeclaration, varName, filename, lineNumber, startCol };
            case Variables::Type::VT_STRING:
                return { TokenType::StringDeclaration, varName, filename, lineNumber, startCol };
            default:
                return { TokenType::Unknown, "Invalid variable type in declaration", filename, lineNumber, startCol };
        }
    } else {
        return { TokenType::Unknown, "$ followed by invalid character in declaration", filename, lineNumber, startCol };
    }
}

Token Lexer::singleCharToken(TokenType type, const std::string & lexeme) {
    size_t startCol = colNumber;
    advance();
    return { type, lexeme, filename, lineNumber, startCol };
}

bool Lexer::matchSequence(const std::string & sequence) const {
    if (pos + sequence.length() > src.length()) {
        return false;
    }
    return src.substr(pos, sequence.length()) == sequence;
}

void Lexer::matchAndConsume(const std::string & sequence) {
    if (matchSequence(sequence)) {
        for (size_t i = 0; i < sequence.length(); ++i) {
            advance();
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        char c = peek();
        if (isspace(c)) {
            advance();
            continue;
        }
        if (c == '\n') {
            tokens.push_back(singleCharToken(TokenType::EndOfLine, ""));
            continue;
        }
        if (c == '#') {
            tokens.push_back(comment());
            advance();  // Skip newline after comment
            continue;
        }
        if (matchSequence(PARSER_OPEN_TAG)) {
            size_t startCol = colNumber;
            matchAndConsume(PARSER_OPEN_TAG);
            tokens.push_back({ TokenType::ParserOpenTag, PARSER_OPEN_TAG, filename, lineNumber, startCol });
            continue;
        }
        if (matchSequence(PARSER_CLOSE_TAG)) {
            size_t startCol = colNumber;
            matchAndConsume(PARSER_CLOSE_TAG);
            tokens.push_back({ TokenType::ParserCloseTag, PARSER_CLOSE_TAG, filename, lineNumber, startCol });
            continue;
        }
        if (isalpha(c)) {
            tokens.push_back(keywordOrIdentifier());
        } else if (c == '$') {
            tokens.push_back(variable());
        } else if (isdigit(c)) {
            tokens.push_back(number());
        } else if (c == '"' || c == '\'') {
            tokens.push_back(string());
        } else if (c == '(') {
            tokens.push_back(singleCharToken(TokenType::LeftParenthesis, "("));
        } else if (c == ')') {
            tokens.push_back(singleCharToken(TokenType::RightParenthesis, ")"));
        } else if (c == ',') {
            tokens.push_back(singleCharToken(TokenType::Comma, ","));
        } else if (c == ';') {
            tokens.push_back(singleCharToken(TokenType::Semicolon, ";"));
        } else if (c == '=') {
            tokens.push_back(singleCharToken(TokenType::Equals, "="));
        } else {
            tokens.push_back({ TokenType::Unknown, std::string(1, c), filename, lineNumber, colNumber });
            advance();
        }
    }

    tokens.push_back({ TokenType::EndOfFile, "", filename, lineNumber, colNumber });
    return tokens;
}
