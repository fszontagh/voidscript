#include "Lexer.hpp"

#include <cctype>

#include "options.h"

Lexer::Lexer(const std::string & source, const std::string & filename) :
    src(source),
    pos(0),
    filename(filename),
    lineNumber(1),
    colNumber(1),
    charNumber(0) {}

/**
 * Peek at the current character without advancing the lexer's position.
 *
 * @return The current character, or '\0' if at the end of the source.
 */
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

Tokens::Token Lexer::createToken(Tokens::Type type, const std::string & lexeme) const {
    size_t startChar = charNumber - lexeme.length();
    return {
        type, lexeme, filename, lineNumber, colNumber - lexeme.length(), { startChar, charNumber }
    };
}

Tokens::Token Lexer::createSingleCharToken(Tokens::Type type, const std::string & lexeme) {
    size_t startCol  = colNumber;
    size_t startChar = charNumber;
    advance();
    return {
        type, lexeme, filename, lineNumber, startCol, { startChar, charNumber }
    };
}

Tokens::Token Lexer::createUnknownToken(const std::string & lexeme) const {
    size_t startChar = charNumber - lexeme.length();
    return {
        Tokens::Type::Unknown, lexeme, filename, lineNumber, colNumber - lexeme.length(), { startChar, charNumber }
    };
}

Tokens::Token Lexer::stringToken() {
    std::string result;
    size_t      startChar = charNumber;
    size_t      startCol  = colNumber;
    advance();  // Skip opening quote
    while (!isAtEnd() && peek() != '"') {
        result += advance();
    }
    if (isAtEnd() || peek() != '"') {
        return {
            Tokens::Type::Unknown, "Unterminated string", filename, lineNumber, startCol, { startChar, pos }
        };
    }
    advance();  // Skip closing quote
    return {
        Tokens::Type::StringLiteral, result, filename, lineNumber, startCol, { startChar, pos }
    };
}

Tokens::Token Lexer::numberToken() {
    std::string  result;
    std::string  found;
    Tokens::Type type             = Tokens::Type::Unknown;
    bool         decimalPointSeen = false;
    size_t       startChar        = charNumber;
    size_t       startCol         = colNumber;

    while (std::isdigit(peek()) || peek() == '.') {
        if (peek() == '.') {
            if (decimalPointSeen) {
                return {
                    Tokens::Type::Unknown, "Invalid number format", filename, lineNumber, startCol, { startChar, pos }
                };
            }
            decimalPointSeen = true;
        }
        found.append(1, advance());
    }

    if (!found.empty()) {
        if (found.find('.') == std::string::npos) {
            if (is_number<int>(found)) {
                result = found;
                type   = Tokens::Type::IntLiteral;
            } else {
                return {
                    Tokens::Type::Unknown, "Invalid integer", filename, lineNumber, startCol, { startChar, pos }
                };
            }
        } else {
            if (is_number<double>(found)) {
                result = found;
                type   = Tokens::Type::DoubleLiteral;
            } else {
                return {
                    Tokens::Type::Unknown, "Invalid double", filename, lineNumber, startCol, { startChar, pos }
                };
            }
        }
    } else {
        return {
            Tokens::Type::Unknown, "Expected number", filename, lineNumber, startCol, { startChar, pos }
        };
    }

    return {
        type, result, filename, lineNumber, startCol, { startChar, pos }
    };
}

Tokens::Token Lexer::identifierToken() {
    std::string result;
    size_t      startChar = charNumber;
    size_t      startCol  = colNumber;
    while (isalnum(peek()) || peek() == '_') {
        result += advance();
    }
    return {
        Tokens::Type::Identifier, result, filename, lineNumber, startCol, { startChar, pos }
    };
}

Tokens::Token Lexer::variableToken() {
    size_t startChar = charNumber;
    size_t startCol  = colNumber;
    advance();  // Skip $
    std::string varName;
    if (isalpha(peek()) || peek() == '_') {
        varName += advance();
        while (isalnum(peek()) || peek() == '_') {
            varName += advance();
        }
        return {
            Tokens::Type::Variable, varName, filename, lineNumber, startCol, { startChar, pos }
        };
    }
    return {
        Tokens::Type::Unknown, "$ followed by invalid character", filename, lineNumber, startCol, { startChar, pos }
    };
}

Tokens::Token Lexer::commentToken() {
    size_t startChar = charNumber;
    size_t startCol  = colNumber;
    advance();  // Skip #
    std::string commentText;
    while (!isAtEnd() && peek() != '\n') {
        commentText += advance();
    }
    return {
        Tokens::Type::Comment, commentText, filename, lineNumber, startCol, { startChar, pos }
    };
}

Tokens::Token Lexer::keywordOrIdentifierToken() {
    std::string lexeme;
    while (isalpha(peek())) {
        lexeme += advance();
    }
    if (lexeme == IDENTIFIER_FUNCTION) {
        return this->functionDeclarationToken();
    }

    if (lexeme == IDENTIFIER_RETURN) {
        return createToken(Tokens::Type::Return, lexeme);
    }
    if (lexeme == IDENTIFIER_IF) {
        return createToken(Tokens::Type::ParserIfStatement, lexeme);
    }

    if (peek() == '(') {  // Function call
        return createToken(Tokens::Type::FunctionCall, lexeme);
    }

    auto it = Variables::StringToTypeMap.find(lexeme);
    if (it != Variables::StringToTypeMap.end()) {
        const auto & type = it->second;
        while (isspace(peek())) {
            advance();
        }

        if (peek() == IDENTIFIER_VARIABLE) {
            return this->variableDeclarationToken(type);
        }
        return createToken(Tokens::Type::Identifier, lexeme);
    }
    return createToken(Tokens::Type::Identifier, lexeme);
}

Tokens::Token Lexer::functionDeclarationToken() {
    advance();  // Skip function
    std::string functionName;
    if (isalpha(peek()) || peek() == '_') {
        functionName += advance();
        while (isalnum(peek()) || peek() == '_') {
            functionName += advance();
        }
        return createToken(Tokens::Type::FunctionDeclaration, functionName);
    }
    return createUnknownToken("function followed by invalid character");
}

Tokens::Token Lexer::variableDeclarationToken(Variables::Type type) {
    advance();  // Skip $
    std::string varName;
    if (isalpha(peek()) || peek() == '_') {
        varName += advance();
        while (isalnum(peek()) || peek() == '_') {
            varName += advance();
        }
        for (auto it = Variables::StringToTypeMap.begin(); it != Variables::StringToTypeMap.end(); ++it) {
            if (it->second == type) {
                return createToken(getTokenTypeFromValueDeclaration(it->second), varName);
            }
        }
        return createUnknownToken("Invalid variable type in declaration");
    }
    return createUnknownToken("$ followed by invalid character in declaration");
}

bool Lexer::matchSequence(const std::string & sequence, bool caseSensitive) const {
    if (this->pos + sequence.size() > src.size()) {
        return false;
    }
    for (size_t i = 0; i < sequence.size(); ++i) {
        char srcChar = src[this->pos + i];
        char seqChar = sequence[i];
        if (!caseSensitive) {
            srcChar = std::tolower(static_cast<unsigned char>(srcChar));
            seqChar = std::tolower(static_cast<unsigned char>(seqChar));
        }
        if (srcChar != seqChar) {
            return false;
        }
    }
    return true;
}

void Lexer::matchAndConsume(const std::string & sequence, bool caseSensitive) {
    if (matchSequence(sequence, caseSensitive)) {
        for (size_t i = 0; i < sequence.length(); ++i) {
            advance();
        }
    }
}

std::vector<Tokens::Token> Lexer::tokenize() {
    std::vector<Tokens::Token> tokens;
    tokens.reserve(src.size() / 4);

    while (pos < src.size()) {
        char c = src[pos];

        if (isspace(c)) {
            advance();
            continue;
        }
        if (c == '\n') {
            tokens.push_back(createSingleCharToken(Tokens::Type::EndOfLine, "\n"));
            continue;
        }
        if (c == IDENTIFIER_COMMENT) {
            tokens.push_back(commentToken());
            advance();  // Skip newline after comment
            continue;
        }
        if (matchSequence(PARSER_OPEN_TAG)) {
            matchAndConsume(PARSER_OPEN_TAG);
            tokens.push_back(createToken(Tokens::Type::ParserOpenTag, PARSER_OPEN_TAG));
            continue;
        }
        if (matchSequence(PARSER_CLOSE_TAG)) {
            matchAndConsume(PARSER_CLOSE_TAG);
            tokens.push_back(createToken(Tokens::Type::ParserCloseTag, PARSER_CLOSE_TAG));
            continue;
        }
        switch (c) {
            case 'a' ... 'z':
            case 'A' ... 'Z':
                tokens.push_back(keywordOrIdentifierToken());
                break;
            case IDENTIFIER_VARIABLE:
                tokens.push_back(variableToken());
                break;
            case '0' ... '9':
                tokens.push_back(numberToken());
                break;
            case '"':
            case '\'':
                tokens.push_back(stringToken());
                break;
            case '(':
                tokens.push_back(createSingleCharToken(Tokens::Type::LeftParenthesis, "("));
                break;
            case ')':
                tokens.push_back(createSingleCharToken(Tokens::Type::RightParenthesis, ")"));
                break;
            case ',':
                tokens.push_back(createSingleCharToken(Tokens::Type::Comma, ","));
                break;
            case ';':
                tokens.push_back(createSingleCharToken(Tokens::Type::Semicolon, ";"));
                break;
            case '=':
                tokens.push_back(createSingleCharToken(Tokens::Type::Equals, "="));
                break;
            case '+':
                tokens.push_back(createSingleCharToken(Tokens::Type::Plus, "+"));
                break;
            case '{':
                tokens.push_back(createSingleCharToken(Tokens::Type::LeftCurlyBracket, "{"));
                break;
            case '}':
                tokens.push_back(createSingleCharToken(Tokens::Type::RightCurlyBracket, "}"));
                break;
            default:
                tokens.push_back(createUnknownToken(std::string(1, c)));
                advance();
                break;
        }
    }

    tokens.push_back({
        Tokens::Type::EndOfFile, "", filename, lineNumber, colNumber, { charNumber, charNumber }
    });
    return tokens;
}
