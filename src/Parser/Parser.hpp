#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include "Interpreter/ExpressionBuilder.hpp"
#include "Interpreter/OperationsFactory.hpp"
#include "Lexer/Token.hpp"
#include "Lexer/TokenType.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

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

    void parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                     const std::string & filename) {
        tokens_              = tokens;
        input_str_view_      = input_string;
        current_token_index_ = 0;
        current_filename_    = filename;

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

    static const std::unordered_map<std::string, Lexer::Tokens::Type>              keywords;
    static const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> variable_types;

  private:
    std::vector<Lexer::Tokens::Token> tokens_;
    std::string_view                  input_str_view_;
    size_t                            current_token_index_;
    std::string                       current_filename_;

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

        if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
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

    void parseVariableDefinition() {
        Symbols::Variables::Type var_type = parseType();

        Lexer::Tokens::Token id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string          var_name = id_token.value;

        if (!var_name.empty() && var_name[0] == '$') {
            var_name = var_name.substr(1);
        }
        const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();

        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        /*
        Symbols::Value initial_value = parseValue(var_type);

        Interpreter::OperationsFactory::defineSimpleVariable(var_name, initial_value, ns, this->current_filename_,
                                                             id_token.line_number, id_token.column_number);
*/

        auto expr = parseParsedExpression(var_type);
        Interpreter::OperationsFactory::defineVariableWithExpression(
            var_name, var_type, std::move(expr), ns, current_filename_, id_token.line_number, id_token.column_number);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
    }

    void parseFunctionDefinition() {
        expect(Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION);
        Lexer::Tokens::Token     id_token         = expect(Lexer::Tokens::Type::IDENTIFIER);
        std::string              func_name        = id_token.value;
        Symbols::Variables::Type func_return_type = Symbols::Variables::Type::NULL_TYPE;
        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        expect(Lexer::Tokens::Type::PUNCTUATION, "(");

        Symbols::FunctionParameterInfo param_infos;

        if (currentToken().type != Lexer::Tokens::Type::PUNCTUATION || currentToken().value != ")") {
            while (true) {
                // Paraméter típusa
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

        Lexer::Tokens::Token opening_brace = expect(Lexer::Tokens::Type::PUNCTUATION, "{");

        // only parse the body if we checked out if not exists the function and created the symbol
        parseFunctionBody(opening_brace, func_name, func_return_type, param_infos);
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

    Symbols::Value parseValue(Symbols::Variables::Type expected_var_type) {
        Lexer::Tokens::Token token       = currentToken();
        bool                 is_negative = false;

        // Előjel kezelése
        if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && (token.lexeme == "-" || token.lexeme == "+") &&
            peekToken().type == Lexer::Tokens::Type::NUMBER) {
            is_negative = (token.lexeme == "-");
            token       = peekToken();
            consumeToken();  // előjelet elfogyasztottuk
        }

        // STRING típus
        if (expected_var_type == Symbols::Variables::Type::STRING) {
            if (token.type == Lexer::Tokens::Type::STRING_LITERAL) {
                consumeToken();
                return Symbols::Value(token.value);
            }
            reportError("Expected string literal value");
        }

        // BOOLEAN típus
        if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
            if (token.type == Lexer::Tokens::Type::KEYWORD && (token.value == "true" || token.value == "false")) {
                consumeToken();
                return Symbols::Value(token.value == "true");
            }
            reportError("Expected boolean literal value (true or false)");
        }

        // NUMERIC típusok
        if (expected_var_type == Symbols::Variables::Type::INTEGER ||
            expected_var_type == Symbols::Variables::Type::DOUBLE ||
            expected_var_type == Symbols::Variables::Type::FLOAT) {
            if (token.type == Lexer::Tokens::Type::NUMBER) {
                Symbols::Value val = parseNumericLiteral(token.value, is_negative, expected_var_type);
                consumeToken();
                return val;
            }

            reportError("Expected numeric literal value");
        }

        reportError("Unsupported variable type encountered during value parsing");
        return Symbols::Value();  // compiler happy
    }

    Symbols::Value parseNumericLiteral(const std::string & value, bool is_negative, Symbols::Variables::Type type) {
        try {
            switch (type) {
                case Symbols::Variables::Type::INTEGER:
                    {
                        if (value.find('.') != std::string::npos) {
                            throw std::invalid_argument("Floating point value in integer context: " + value);
                        }
                        int v = std::stoi(value);
                        return Symbols::Value(is_negative ? -v : v);
                    }
                case Symbols::Variables::Type::DOUBLE:
                    {
                        double v = std::stod(value);
                        return Symbols::Value(is_negative ? -v : v);
                    }
                case Symbols::Variables::Type::FLOAT:
                    {
                        float v = std::stof(value);
                        return Symbols::Value(is_negative ? -v : v);
                    }
                default:
                    throw std::invalid_argument("Unsupported numeric type");
            }
        } catch (const std::invalid_argument & e) {
            reportError("Invalid numeric literal: " + value + " (" + e.what() + ")");
        } catch (const std::out_of_range & e) {
            reportError("Numeric literal out of range: " + value + " (" + e.what() + ")");
        }

        return Symbols::Value();  // unreachable
    }

    void parseFunctionBody(const Lexer::Tokens::Token & opening_brace, const std::string & function_name,
                           Symbols::Variables::Type return_type, const Symbols::FunctionParameterInfo & params) {
        size_t               braceDepth = 0;
        int                  peek       = 0;
        int                  tokenIndex = current_token_index_;
        Lexer::Tokens::Token currentToken_;
        Lexer::Tokens::Token closing_brace;

        while (tokenIndex < tokens_.size()) {
            currentToken_ = peekToken(peek);
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
            tokenIndex++;
            peek++;
        }
        if (braceDepth != 0) {
            reportError("Unmatched braces in function body");
        }
        std::vector<Lexer::Tokens::Token> filtered_tokens;
        auto                              startIt = std::find(tokens_.begin(), tokens_.end(), opening_brace);
        auto                              endIt   = std::find(tokens_.begin(), tokens_.end(), closing_brace);

        if (startIt != tokens_.end() && endIt != tokens_.end() && startIt < endIt) {
            filtered_tokens = std::vector<Lexer::Tokens::Token>(startIt + 1, endIt);
        }
        std::string_view input_string = input_str_view_.substr(opening_brace.end_pos, closing_brace.end_pos);

        current_token_index_ = tokenIndex;
        expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        const std::string newns = Symbols::SymbolContainer::instance()->currentScopeName() + "." + function_name;
        Symbols::SymbolContainer::instance()->create(newns);
        std::shared_ptr<Parser> parser = std::make_shared<Parser>();
        parser->parseScript(filtered_tokens, input_string, this->current_filename_);
        Symbols::SymbolContainer::instance()->enterPreviousScope();
        // create function
        Interpreter::OperationsFactory::defineFunction(
            function_name, params, return_type, Symbols::SymbolContainer::instance()->currentScopeName(),
            this->current_filename_, currentToken_.line_number, currentToken_.column_number);
    }

    ParsedExpressionPtr parseParsedExpression(const Symbols::Variables::Type & expected_var_type) {
        std::stack<std::string>          operator_stack;
        std::vector<ParsedExpressionPtr> output_queue;

        auto getPrecedence = [](const std::string & op) -> int {
            if (op == "+" || op == "-") {
                return 1;
            }
            if (op == "*" || op == "/") {
                return 2;
            }
            if (op == "u-" || op == "u+") {
                return 3;
            }
            return 0;
        };

        auto isLeftAssociative = [](const std::string & op) -> bool {
            return !(op == "u-" || op == "u+");
        };

        auto applyOperator = [](const std::string & op, ParsedExpressionPtr rhs, ParsedExpressionPtr lhs = nullptr) {
            if (op == "u-" || op == "u+") {
                std::string real_op = (op == "u-") ? "-" : "+";
                return ParsedExpression::makeUnary(real_op, std::move(rhs));
            } else {
                return ParsedExpression::makeBinary(op, std::move(lhs), std::move(rhs));
            }
        };

        auto pushOperand = [&](const Lexer::Tokens::Token & token) {
            if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                token.type == Lexer::Tokens::Type::KEYWORD) {
                output_queue.push_back(
                    ParsedExpression::makeLiteral(Symbols::Value::fromString(token.value, expected_var_type)));
            } else if (token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                std::string name = token.value;
                if (!name.empty() && name[0] == '$') {
                    name = name.substr(1);
                }
                output_queue.push_back(ParsedExpression::makeVariable(name));
            } else {
                reportError("Expected literal or variable");
            }
        };

        bool expect_unary = true;

        while (true) {
            auto token = currentToken();

            if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "(") {
                operator_stack.push("(");
                consumeToken();
                expect_unary = true;
            } else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == ")") {
                consumeToken();
                while (!operator_stack.empty() && operator_stack.top() != "(") {
                    std::string op = operator_stack.top();
                    operator_stack.pop();

                    if (op == "u-" || op == "u+") {
                        if (output_queue.empty()) {
                            reportError("Missing operand for unary operator");
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(applyOperator(op, std::move(rhs)));
                    } else {
                        if (output_queue.size() < 2) {
                            reportError("Malformed expression");
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        auto lhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(applyOperator(op, std::move(rhs), std::move(lhs)));
                    }
                }

                if (operator_stack.empty() || operator_stack.top() != "(") {
                    reportError("Mismatched parentheses");
                }
                operator_stack.pop();  // remove "("
                expect_unary = false;
            } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC) {
                std::string op = std::string(token.lexeme);
                if (expect_unary && (op == "-" || op == "+")) {
                    op = "u" + op;  // pl. u-
                }

                while (!operator_stack.empty()) {
                    const std::string & top = operator_stack.top();
                    if ((isLeftAssociative(op) && getPrecedence(op) <= getPrecedence(top)) ||
                        (!isLeftAssociative(op) && getPrecedence(op) < getPrecedence(top))) {
                        operator_stack.pop();

                        if (top == "u-" || top == "u+") {
                            if (output_queue.empty()) {
                                reportError("Missing operand for unary operator");
                            }
                            auto rhs = std::move(output_queue.back());
                            output_queue.pop_back();
                            output_queue.push_back(applyOperator(top, std::move(rhs)));
                        } else {
                            if (output_queue.size() < 2) {
                                reportError("Malformed expression");
                            }
                            auto rhs = std::move(output_queue.back());
                            output_queue.pop_back();
                            auto lhs = std::move(output_queue.back());
                            output_queue.pop_back();
                            output_queue.push_back(applyOperator(top, std::move(rhs), std::move(lhs)));
                        }
                    } else {
                        break;
                    }
                }

                operator_stack.push(op);
                consumeToken();
                expect_unary = true;
            } else if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                       token.type == Lexer::Tokens::Type::KEYWORD ||
                       token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                pushOperand(token);
                consumeToken();
                expect_unary = false;
            } else {
                break;
            }
        }

        // Kiürítjük az operator stack-et
        while (!operator_stack.empty()) {
            std::string op = operator_stack.top();
            operator_stack.pop();

            if (op == "(" || op == ")") {
                reportError("Mismatched parentheses");
            }

            if (op == "u-" || op == "u+") {
                if (output_queue.empty()) {
                    reportError("Missing operand for unary operator");
                }
                auto rhs = std::move(output_queue.back());
                output_queue.pop_back();
                output_queue.push_back(applyOperator(op, std::move(rhs)));
            } else {
                if (output_queue.size() < 2) {
                    reportError("Malformed expression");
                }
                auto rhs = std::move(output_queue.back());
                output_queue.pop_back();
                auto lhs = std::move(output_queue.back());
                output_queue.pop_back();
                output_queue.push_back(applyOperator(op, std::move(rhs), std::move(lhs)));
            }
        }

        if (output_queue.size() != 1) {
            reportError("Expression could not be parsed cleanly");
        }

        return std::move(output_queue.back());
    }

};  // class Parser

}  // namespace Parser

#endif  // PARSER_HPP
