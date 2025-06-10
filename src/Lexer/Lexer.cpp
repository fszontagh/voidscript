#include "Lexer/Lexer.hpp"

#include "Lexer/Operators.hpp"
#include "Symbols/SymbolContainer.hpp"

std::vector<Lexer::Tokens::Token> Lexer::Lexer::tokenizeNamespace(const std::string & ns) {
    if (inputs_.find(ns) == inputs_.end()) {
        return {};
    }

    Symbols::SymbolContainer::instance()->enter(ns);

    std::vector<Tokens::Token> tokens;
    Tokens::Token              token;
    do {
        token = nextToken();
        tokens.push_back(token);
    } while (token.type != Tokens::Type::END_OF_FILE);

    tokens_[ns] = tokens;
    return tokens;
}

void Lexer::Lexer::addNamespaceInput(const std::string & ns, const std::string & input) {
    inputs_[ns]         = input;
    positions_[ns]      = 0;
    line_numbers_[ns]   = 1;
    column_numbers_[ns] = 1;
}

std::vector<Lexer::Tokens::Token> Lexer::Lexer::getTokens(const std::string & ns) const {
    auto it = tokens_.find(ns);
    if (it != tokens_.end()) {
        return it->second;
    }
    return {};
}

void Lexer::Lexer::setKeyWords(const std::unordered_map<std::string, Tokens::Type> & new_keywords) {
    keywords = new_keywords;
}

Lexer::Tokens::Token Lexer::Lexer::nextToken() {
    skipWhitespaceAndComments();
    size_t start = pos();

    if (isAtEnd()) {
        return createToken(Tokens::Type::END_OF_FILE, start, start);
    }

    char c = peek();
    if (isalpha(c) || c == '_') {
        return matchIdentifierOrKeyword(start);
    }
    if (isdigit(c) || (isdigit(c) && peek(1) == '.') || (c == '.' && isdigit(peek(1)))) {
        return matchNumber(start);
    }
    if (c == '"' || c == '\'') {
        return matchStringLiteral(start);
    }
    if (operators_.find(c) != std::string_view::npos) {
        return matchOperatorOrPunctuation(start);
    }

    advance();
    return createToken(Tokens::Type::UNKNOWN, start, pos());
}

const std::string & Lexer::Lexer::input() const {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    auto         it = inputs_.find(ns);
    if (it != inputs_.end()) {
        return it->second;
    }
    throw Exception("Input not found in namespace: " + ns);
}

size_t & Lexer::Lexer::pos() {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    auto         it = positions_.find(ns);
    if (it != positions_.end()) {
        return it->second;
    }
    throw Exception("Unknown position in namespace: " + ns);
}

int & Lexer::Lexer::line() {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    auto         it = line_numbers_.find(ns);
    if (it != line_numbers_.end()) {
        return it->second;
    }
    throw Exception("Unknown line number in namespace: " + ns);
}

int & Lexer::Lexer::col() {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    auto         it = column_numbers_.find(ns);
    if (it != column_numbers_.end()) {
        return it->second;
    }
    throw Exception("Unknown column number in namespace: " + ns);
}

Lexer::Tokens::Token Lexer::Lexer::createToken(Tokens::Type type, size_t start, size_t end, const std::string & value) {
    Tokens::Token token;
    token.type          = type;
    token.start_pos     = start;
    token.end_pos       = end;
    token.line_number   = line();
    token.column_number = col();
    if (start <= end && end <= input().length()) {
        token.lexeme = std::string_view(input()).substr(start, end - start);
        token.value  = value.empty() ? std::string(token.lexeme) : value;
    }
    return token;
}

char Lexer::Lexer::peek(size_t offset) const {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    const auto & in = inputs_.at(ns);
    size_t       cp = positions_.at(ns);
    if (cp + offset >= in.length()) {
        return '\0';
    }
    return in[cp + offset];
}

char Lexer::Lexer::advance() {
    char c = peek();
    pos()++;
    if (c == '\n') {
        line()++;
        col() = 1;
    } else {
        col()++;
    }
    return c;
}

bool Lexer::Lexer::isAtEnd() const {
    const auto & ns = Symbols::SymbolContainer::instance()->currentScopeName();
    return positions_.at(ns) >= inputs_.at(ns).length();
}

void Lexer::Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (isspace(c)) {
            advance();
        } else if ((c == '/' && peek(1) == '/') || c == '#') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Lexer::Tokens::Token Lexer::Lexer::matchIdentifierOrKeyword(size_t start_pos, Tokens::Type type) {
    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {
        advance();
    }
    size_t      end   = pos();
    std::string value = input().substr(start_pos, end - start_pos);
    if (value.empty()) {
        return createToken(Tokens::Type::UNKNOWN, start_pos, end);
    }

    if (type == Tokens::Type::IDENTIFIER) {
        auto it = keywords.find(value);
        if (it != keywords.end()) {
            return createToken(it->second, start_pos, end);
        }
    }
    return createToken(type, start_pos, end);
}

Lexer::Tokens::Token Lexer::Lexer::matchNumber(size_t start_pos) {
    bool has_dot = false;

    while (!isAtEnd()) {
        if (isdigit(peek())) {
            advance();
        } else if (!has_dot && peek() == '.' && isdigit(peek(1))) {
            has_dot = true;
            advance();  // a pont
            advance();  // az első számjegy a pont után
        } else {
            break;
        }
    }

    size_t end = pos();
    return createToken(Tokens::Type::NUMBER, start_pos, end);
}

Lexer::Tokens::Token Lexer::Lexer::matchStringLiteral(size_t start_pos) {
    char opening_quote = peek();
    advance();  // Skip opening quote
    std::string value;
    bool        unterminated = false;

    while (!isAtEnd()) {
        char c = peek();
        if (c == opening_quote) {
            advance();
            break;
        }
        if (c == '\\') {
            advance();
            char e = advance();
            switch (e) {
                case 'n':
                    value += '\n';
                    break;
                case 't':
                    value += '\t';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case 'b':
                    value += '\b';
                    break;
                case 'f':
                    value += '\f';
                    break;
                case 'v':
                    value += '\v';
                    break;
                case 'a':
                    value += '\a';
                    break;
                case '0':
                    value += '\0';
                    break;
                case '"':
                    value += '"';
                    break;
                case '\'':
                    value += '\'';
                    break;
                case '\\':
                    value += '\\';
                    break;
                default:
                    value += e;
                    break;
            }
        } else {
            value += advance();
        }
    }

    size_t end = pos();
    if (unterminated) {
        return createToken(Tokens::Type::UNKNOWN, start_pos, end, input().substr(start_pos, end - start_pos));
    }
    return createToken(Tokens::Type::STRING_LITERAL, start_pos, end, value);
}

Lexer::Tokens::Token Lexer::Lexer::matchOperatorOrPunctuation(size_t start_pos) {
    char first_char = advance();  // Első karakter elfogyasztása

    if (!isAtEnd()) {
        char        second_char = peek(0);  // Következő karakter megnézése
        std::string two_chars_str{ first_char, second_char };

        const std::vector<std::pair<const std::vector<std::string> *, Tokens::Type>> two_char_op_types = {
            { &OPERATOR_RELATIONAL, Tokens::Type::OPERATOR_RELATIONAL },
            { &OPERATOR_INCREMENT,  Tokens::Type::OPERATOR_INCREMENT  },
            { &OPERATOR_ASSIGNMENT, Tokens::Type::OPERATOR_ASSIGNMENT },
            { &OPERATOR_LOGICAL,    Tokens::Type::OPERATOR_LOGICAL    },
            { &PUNCTUATION,         Tokens::Type::PUNCTUATION         }
        };

        for (const auto & [vec_ptr, type] : two_char_op_types) {
            if (matchFromVector(*vec_ptr, two_chars_str)) {
                advance();  // Második karakter elfogyasztása
                size_t end_pos = pos();
                return createToken(type, start_pos, end_pos);
            }
        }
    }

    std::string single_char_str(1, first_char);

    if (single_char_str == "$") {
        if (isalpha(peek(0)) || peek(0) == '_') {
            return matchIdentifierOrKeyword(start_pos, Tokens::Type::VARIABLE_IDENTIFIER);
        }
    }

    // Single-character operator or punctuation tokens
    const std::vector<std::pair<const std::vector<std::string> *, Tokens::Type>> one_char_op_types = {
        { &OPERATOR_ARITHMETIC, Tokens::Type::OPERATOR_ARITHMETIC },
        { &OPERATOR_RELATIONAL, Tokens::Type::OPERATOR_RELATIONAL },
        { &OPERATOR_ASSIGNMENT, Tokens::Type::OPERATOR_ASSIGNMENT },
        { &PUNCTUATION,         Tokens::Type::PUNCTUATION         }
    };

    for (const auto & [vec_ptr, type] : one_char_op_types) {
        if (matchFromVector(*vec_ptr, single_char_str)) {
            size_t end_pos = pos();
            return createToken(type, start_pos, end_pos);
        }
    }

    size_t end_pos = pos();
    return createToken(Tokens::Type::UNKNOWN, start_pos, end_pos);
}


Lexer::Lexer::Lexer() {
    for (const auto & vecRef :
         { std::cref(OPERATOR_ARITHMETIC), std::cref(OPERATOR_RELATIONAL), std::cref(OPERATOR_INCREMENT),
           std::cref(OPERATOR_ASSIGNMENT), std::cref(OPERATOR_LOGICAL), std::cref(PUNCTUATION) }) {
        for (const auto & str : vecRef.get()) {
            operators_ += str;
        }
    }

    operators_ += "$";

    // Initialize keywords
    keywords["enum"]    = Tokens::Type::KEYWORD_ENUM;
    keywords["switch"]  = Tokens::Type::KEYWORD_SWITCH;
    keywords["case"]    = Tokens::Type::KEYWORD_CASE;
    keywords["default"] = Tokens::Type::KEYWORD_DEFAULT;
    keywords["break"]   = Tokens::Type::KEYWORD_BREAK;
    keywords["auto"]    = Tokens::Type::KEYWORD_AUTO;
}
