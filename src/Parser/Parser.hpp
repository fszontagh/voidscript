#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <memory>
#include <sstream>  // Hibaüzenetekhez
#include <stdexcept>
#include <string>
#include <vector>

// Szükséges header-ök a Lexer és Symbol komponensekből
#include "Lexer/Token.hpp"
#include "Lexer/TokenType.hpp"  // Enum és TypeToString
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"  // Symbols::Value miatt

namespace Parser {

class SyntaxError : public std::runtime_error {
  public:
    SyntaxError(const std::string & message, const int line, const int col) :
        std::runtime_error(message + " at line " + std::to_string(line) + ", column " + std::to_string(col)) {}

    SyntaxError(const std::string & message, const Lexer::Tokens::Token & token) :
        SyntaxError(
            message + " (found token: '" + token.value + "' type: " + Lexer::Tokens::TypeToString(token.type) + ")",
            token.line_number, token.column_number) {}
};

class Parser {
  public:
    Parser() {}

    void parseProgram(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string) {
        tokens_              = tokens;
        input_str_view_      = input_string;
        current_token_index_ = 0;
        symbol_container_    = std::make_unique<Symbols::SymbolContainer>();
        try {
            while (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
                parseStatement();
            }
            if (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
                reportError("Unexpected tokens after program end");
            }
        } catch (const SyntaxError & e) {
            std::cerr << "Syntax Error: " << e.what() << '\n';
        } catch (const std::exception & e) {
            std::cerr << "Error during parsing: " << e.what() << '\n';
            throw;
        }
    }

    const std::shared_ptr<Symbols::SymbolContainer> & getSymbolContainer() const {
        if (!symbol_container_) {
            throw std::runtime_error("Symbol container is not initialized.");
        }
        return symbol_container_;
    }

    static const std::unordered_map<std::string, Lexer::Tokens::Type>              keywords;
    static const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> variable_types;

  private:
    std::vector<Lexer::Tokens::Token>         tokens_;
    std::string_view                          input_str_view_;
    size_t                                    current_token_index_;
    std::shared_ptr<Symbols::SymbolContainer> symbol_container_;

    // Token Stream Kezelő és Hibakezelő segédfüggvények (változatlanok)
    const Lexer::Tokens::Token & currentToken() const {
        if (isAtEnd()) {
            // Technikailag itt már nem kellene lennünk, ha a parseProgram ciklus jól van megírva
            // De biztonsági ellenőrzésként jó lehet
            if (!tokens_.empty() && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE) {
                return tokens_.back();  // Visszaadjuk az EOF tokent
            }
            throw std::runtime_error("Unexpected end of token stream reached.");
        }
        return tokens_[current_token_index_];
    }

    // Előre néz a token stream-ben
    const Lexer::Tokens::Token & peekToken(size_t offset = 1) const {
        if (current_token_index_ + offset >= tokens_.size()) {
            // EOF vagy azon túl vagyunk, adjuk vissza az utolsó tokent (ami EOF kell legyen)
            if (!tokens_.empty()) {
                return tokens_.back();
            }
            throw std::runtime_error("Cannot peek beyond end of token stream.");
        }
        return tokens_[current_token_index_ + offset];
    }

    // Elfogyasztja (lépteti az indexet) az aktuális tokent és visszaadja azt
    Lexer::Tokens::Token consumeToken() {
        if (isAtEnd()) {
            throw std::runtime_error("Cannot consume token at end of stream.");
        }
        return tokens_[current_token_index_++];
    }

    // Ellenőrzi, hogy az aktuális token típusa megegyezik-e a várttal.
    // Ha igen, elfogyasztja és true-t ad vissza. Ha nem, false-t ad vissza.
    bool match(Lexer::Tokens::Type expected_type) {
        if (isAtEnd()) {
            return false;
        }
        if (currentToken().type == expected_type) {
            consumeToken();
            return true;
        }
        return false;
    }

    // Ellenőrzi, hogy az aktuális token típusa és értéke megegyezik-e a várttal.
    // Csak OPERATOR és PUNCTUATION esetén érdemes használni az érték ellenőrzést.
    bool match(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
        if (isAtEnd()) {
            return false;
        }
        const auto & token = currentToken();
        if (token.type == expected_type && token.value == expected_value) {
            consumeToken();
            return true;
        }
        return false;
    }

    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type) {
        if (isAtEnd()) {
            reportError("Unexpected end of file, expected token type: " + Lexer::Tokens::TypeToString(expected_type));
        }
        const auto & token = currentToken();
        if (token.type == expected_type) {
            return consumeToken();
        }
        reportError("Expected token type " + Lexer::Tokens::TypeToString(expected_type));
        // A reportError dob, ez a return sosem fut le, de a fordító kedvéért kellhet:
        return token;  // Vagy dobjon a reportError
    }

    // Mint az expect, de az értékét is ellenőrzi.
    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
        if (isAtEnd()) {
            reportError("Unexpected end of file, expected token: " + Lexer::Tokens::TypeToString(expected_type) +
                        " with value '" + expected_value + "'");
        }
        const auto & token = currentToken();
        if (token.type == expected_type && token.value == expected_value) {
            return consumeToken();
        }
        reportError("Expected token " + Lexer::Tokens::TypeToString(expected_type) + " with value '" + expected_value +
                    "'");
        return token;  // reportError dob
    }

    // Ellenőrzi, hogy a releváns tokenek végére értünk-e (az EOF előtti utolsó tokenen vagyunk-e)
    bool isAtEnd() const {
        // Akkor vagyunk a végén, ha az index a tokenek méretével egyenlő,
        // vagy ha már csak az EOF token van hátra (ha az a lista utolsó eleme).
        return current_token_index_ >= tokens_.size() ||
               (current_token_index_ == tokens_.size() - 1 && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE);
    }

    // --- Hibakezelés ---
    // Hiba jelentése és kivétel dobása
    [[noreturn]] void reportError(const std::string & message) {
        // Használjuk az aktuális token pozícióját, ha még nem értünk a végére
        if (current_token_index_ < tokens_.size()) {
            throw SyntaxError(message, tokens_[current_token_index_]);
        }  // Ha már a végén vagyunk, az utolsó ismert pozíciót használjuk
        int line = tokens_.empty() ? 0 : tokens_.back().line_number;
        int col  = tokens_.empty() ? 0 : tokens_.back().column_number;
        throw SyntaxError(message, line, col);
    }

    // --- Elemzési Módszerek (Moduláris részek) ---

    // parseStatement (változatlan)
    void parseStatement() {
        const auto & token_type = currentToken().type;

        if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION) {
            parseFunctionDefinition();
            return;
        }

        for (const auto & _type : Parser::Parser::variable_types) {
            if (token_type == _type.first) {
                parseVariableDefinition();
                return;
            }
        }

        reportError("Unexpected token at beginning of statement");
    }

    // parseVariableDefinition (SymbolFactory használata már korrekt volt)
    void parseVariableDefinition() {
        Symbols::Variables::Type var_type_enum = parseType();
        // A típus stringjének tárolása csak a debug kiíráshoz kell
        //std::string              var_type_str  = tokens_[current_token_index_ - 1].value;
        std::cout << "var_name: " << currentToken().lexeme << std::endl;
        Lexer::Tokens::Token id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string          var_name = id_token.value;
        // Levágjuk a '$' jelet, ha a lexer mégis benne hagyta
        if (!var_name.empty() && var_name[0] == '$') {
            var_name = var_name.substr(1);
        }

        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        Symbols::Value initial_value = parseValue(var_type_enum);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");

        std::string context  = "global";  // Globális változó
        // SymbolFactory használata a létrehozáshoz
        auto variable_symbol = Symbols::SymbolFactory::createVariable(var_name, initial_value, context, var_type_enum);

        // Ellenőrzés és definíció az *aktuális* scope-ban (ami itt a globális)
        if (symbol_container_->exists("variables", var_name)) {
            reportError("Variable '" + var_name + "' already defined in this scope");
        }
        symbol_container_->define("variables", variable_symbol);

        // Debugging kiírás (változatlan)
        // std::cout << "Parsed variable: " << var_type_str << " " << id_token.value << " = ... ;\n";
    }

    // *** MÓDOSÍTOTT parseFunctionDefinition ***
    void parseFunctionDefinition() {
        expect(Lexer::Tokens::Type::KEYWORD_FUNCTION);
        Lexer::Tokens::Token     id_token         = expect(Lexer::Tokens::Type::IDENTIFIER);
        std::string              func_name        = id_token.value;
        Symbols::Variables::Type func_return_type = Symbols::Variables::Type::NULL_TYPE;
        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        expect(Lexer::Tokens::Type::PUNCTUATION, "(");

        Symbols::FunctionParameterInfo param_infos;

        if (currentToken().type != Lexer::Tokens::Type::PUNCTUATION || currentToken().value != ")") {
            while (true) {
                // Paraméter típusa
                //size_t                   type_token_index = current_token_index_;  // Elmentjük a típus token indexét
                Symbols::Variables::Type param_type = parseType();  // Ez elfogyasztja a type tokent

                // Paraméter név ($variable)
                Lexer::Tokens::Token param_id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
                std::string          param_name     = param_id_token.value;
                if (!param_name.empty() && param_name[0] == '$') {  // '$' eltávolítása
                    param_name = param_name.substr(1);
                }

                param_infos.push_back({ param_name, param_type });

                // Vessző vagy zárójel következik?
                if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                    continue;
                }
                if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")") {
                    break;  // Lista vége
                }
                reportError("Expected ',' or ')' in parameter list");
            }
        }
        // Most a ')' következik
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");

        // check if we have a option return type: function name() type { ... }
        for (const auto & _type : Parser::variable_types) {
            if (match(_type.first)) {
                func_return_type = _type.second;
                break;
            }
        }

        auto function_symbol =
            Symbols::SymbolFactory::createFunction(func_name, "global", param_infos, "", func_return_type);
        if (symbol_container_->exists("functions", func_name)) {
            reportError("Function '" + func_name + "' already defined in this scope");
        }

        Lexer::Tokens::Token opening_brace = expect(Lexer::Tokens::Type::PUNCTUATION, "{");

        symbol_container_->define("functions", function_symbol);

        // only parse the body if we checked out if not exists the function and created the symbol
        Symbols::SymbolContainer func_symbols =
            parseFunctionBody(opening_brace, func_return_type != Symbols::Variables::Type::NULL_TYPE);

        // create new container for the function

        std::cout << "Defined function symbol: " << func_name << " in global scope.\n";

        // 3. Új scope nyitása a függvény paraméterei (és később lokális változói) számára
        symbol_container_->enterScope();
        //std::cout << "Entered scope for function: " << func_name << "\n";

        // 4. Paraméterek definiálása mint változók az *új* (függvény) scope-ban
        const std::string & param_context = func_name;  // Paraméter kontextusa a függvény neve
        for (const auto & p_info : param_infos) {
            auto param_symbol = Symbols::SymbolFactory::createVariable(p_info.name,       // Név '$' nélkül
                                                                       Symbols::Value(),  // Alapértelmezett/üres érték
                                                                       param_context,     // Kontextus
                                                                       p_info.type        // Típus
            );

            if (symbol_container_->exists("variables", p_info.name)) {
                reportError("Parameter name '" + p_info.name + "' conflicts with another symbol in function '" +
                            func_name + "'");
            }
            symbol_container_->define("variables", param_symbol);
        }

        // 5. Függvény scope elhagyása
        symbol_container_->leaveScope();
        //std::cout << "Left scope for function: " << func_name << "\n";

        // Debugging kiírás (változatlan)
        // std::cout << "Parsed function: " << func_name << " (...) { ... } scope handled.\n";
    }

    // --- Elemzési Segédfüggvények ---

    // type : KEYWORD_STRING | KEYWORD_INT | KEYWORD_DOUBLE
    // Visszaadja a megfelelő Symbols::Variables::Type enum értéket és elfogyasztja a tokent.
    Symbols::Variables::Type parseType() {
        const auto & token = currentToken();
        for (const auto & _type : Parser::variable_types) {
            if (token.type == _type.first) {
                consumeToken();
                return _type.second;
            }
        }

        reportError("Expected type keyword (string, int, double, float)");
    }

    // value : STRING_LITERAL | NUMBER
    // Feldolgozza az értéket a várt típus alapján.
    Symbols::Value parseValue(Symbols::Variables::Type expected_var_type) {
        Lexer::Tokens::Token token = currentToken();

        /// find if this is a function call
        if (token.type == Lexer::Tokens::Type::IDENTIFIER && peekToken().lexeme == "(") {
            for (const auto & symbol_ptr : this->symbol_container_->listNamespace("functions")) {
                if (auto func_symbol = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(symbol_ptr)) {
                    // TODO: A függvény hívását kellene feldolgozni, a func_symbol-ból kellene lekérni a paramétereket, func_symbol->plainBody() tartalmazza a függvény törzsét
                    // A függvény hívásának feldolgozása
                    std::cout << "Function call: " << token.value << "\n";
                    while (consumeToken().lexeme != ")") {
                        // Feltételezzük, hogy a függvény hívását a lexer már feldolgozta
                    }
                    return Symbols::Value("");  // TODO: Implementálni a függvény hívását
                }
            }
        }

        bool is_negative = false;
        std::cout << "Peek: " << peekToken().lexeme << "\n";
        if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && peekToken().type == Lexer::Tokens::Type::NUMBER) {
            is_negative = true;
            token       = peekToken();
            consumeToken();
        }

        if (expected_var_type == Symbols::Variables::Type::STRING) {
            if (token.type == Lexer::Tokens::Type::STRING_LITERAL) {
                consumeToken();
                return Symbols::Value(token.value);  // A lexer value-ja már a feldolgozott string
            }
            reportError("Expected string literal value");
        } else if (expected_var_type == Symbols::Variables::Type::INTEGER) {
            if (token.type == Lexer::Tokens::Type::NUMBER) {
                // Konvertálás int-re, hibakezeléssel
                try {
                    // TODO: Ellenőrizni, hogy a szám valóban egész-e (nincs benne '.')
                    // Most egyszerűen std::stoi-t használunk.
                    int int_value = std::stoi(token.value);
                    if (is_negative) {
                        int_value = -int_value;
                    }
                    consumeToken();
                    return Symbols::Value(int_value);
                } catch (const std::invalid_argument & e) {
                    reportError("Invalid integer literal: " + token.value);
                } catch (const std::out_of_range & e) {
                    reportError("Integer literal out of range: " + token.value);
                }
            }
            reportError("Expected integer literal value");
        } else if (expected_var_type == Symbols::Variables::Type::DOUBLE) {
            if (token.type == Lexer::Tokens::Type::NUMBER) {
                // Konvertálás double-re, hibakezeléssel
                try {
                    double double_value = std::stod(token.value);
                    if (is_negative) {
                        double_value = -double_value;
                    }
                    consumeToken();
                    return Symbols::Value(double_value);
                } catch (const std::invalid_argument & e) {
                    reportError("Invalid double literal: " + token.value);
                } catch (const std::out_of_range & e) {
                    reportError("Double literal out of range: " + token.value);
                }
            }
            reportError("Expected numeric literal value for double");
        } else if (expected_var_type == Symbols::Variables::Type::FLOAT) {
            if (token.type == Lexer::Tokens::Type::NUMBER) {
                // Konvertálás double-re, hibakezeléssel
                try {

                    float float_value = std::atof(token.value.data());
                    if (is_negative) {
                        float_value = -float_value;
                    }
                    consumeToken();
                    return Symbols::Value(float_value);
                } catch (const std::invalid_argument & e) {
                    reportError("Invalid float literal: " + token.value);
                } catch (const std::out_of_range & e) {
                    reportError("Float literal out of range: " + token.value);
                }
            }
            reportError("Expected numeric literal value for double");
        } else if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
            if (token.type == Lexer::Tokens::Type::KEYWORD) {
                consumeToken();
                return Symbols::Value(token.value == "true");  // A lexer value-ja már a feldolgozott string
            }
            reportError("Expected boolean literal value");
        } else {
            // Más típusok (pl. boolean) itt kezelendők, ha lennének
            reportError("Unsupported variable type encountered during value parsing");
        }
        // Should not reach here due to reportError throwing
        return Symbols::Value();  // Default return to satisfy compiler
    }

    Symbols::SymbolContainer parseFunctionBody(const Lexer::Tokens::Token & opening_brace,
                                               bool                         return_required = false) {
        size_t               braceDepth = 0;
        int                  tokenIndex = current_token_index_;
        Lexer::Tokens::Token currentToken_;
        Lexer::Tokens::Token closing_brace;

        while (tokenIndex < tokens_.size()) {
            currentToken_ = peekToken();

            if (currentToken_.type == Lexer::Tokens::Type::PUNCTUATION) {
                if (currentToken_.value == "{") {
                    ++braceDepth;
                } else if (currentToken_.value == "}") {
                    if (braceDepth == 0) {
                        closing_brace = currentToken_;
                        break;
                    }
                    --braceDepth;
                }
            }
        }
        if (braceDepth != 0) {
            reportError("Unmatched braces in function body");
        }
        std::vector<Lexer::Tokens::Token> filtered_tokens;
        auto                              startIt = std::find(tokens_.begin(), tokens_.end(), opening_brace);
        auto                              endIt   = std::find(tokens_.begin(), tokens_.end(), closing_brace);

        // Ellenőrzés: mindkét token megtalálva és start_token megelőzi az end_token-t
        if (startIt != tokens_.end() && endIt != tokens_.end() && startIt < endIt) {
            filtered_tokens = std::vector<Lexer::Tokens::Token>(startIt + 1, endIt);
        }
        std::string_view input_string = input_str_view_.substr(opening_brace.end_pos, closing_brace.end_pos);
        auto             parser       = Parser();
        parser.parseProgram(filtered_tokens, input_string);

        return *parser.getSymbolContainer();
    }

    std::string parseFunctionBodyOld(size_t body_start_pos, bool return_required = false) {
        std::stringstream body_ss;
        int               brace_level        = 1;
        size_t            last_token_end_pos = body_start_pos;
        bool              has_return         = false;

        while (true) {
            if (isAtEnd()) {
                reportError("Unexpected end of file inside function body (missing '}')");
            }
            const auto & token = currentToken();

            // Whitespace visszaállítása (ha volt) az 'input_str_view_' alapján
            if (token.start_pos > last_token_end_pos) {
                if (last_token_end_pos < input_str_view_.length() && token.start_pos <= input_str_view_.length()) {
                    body_ss << input_str_view_.substr(last_token_end_pos, token.start_pos - last_token_end_pos);
                } else {
                    reportError("Invalid position range in body reconstruction");
                }
            }

            if (token.type == Lexer::Tokens::Type::KEYWORD_RETURN) {
                has_return = true;
            }

            if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "{") {
                brace_level++;
                body_ss << token.lexeme;
            } else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "}") {
                brace_level--;
                if (brace_level == 0) {
                    consumeToken();  // Záró '}' elfogyasztása
                    break;
                }
                body_ss << token.lexeme;
            } else {
                body_ss << token.lexeme;
            }

            last_token_end_pos = token.end_pos;
            consumeToken();
        }

        if (return_required && !has_return) {
            return "";
        }

        return body_ss.str();
    }

};  // class Parser

}  // namespace Parser

#endif  // PARSER_HPP
