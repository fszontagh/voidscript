#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include "BaseException.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Lexer/Token.hpp"
#include "Lexer/TokenType.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/Value.hpp"

namespace Parser {

class Parser {
  public:
    Parser() {}

    class Exception : public BaseException {
      public:
        using BaseException::BaseException;
        // Filename for error reporting
        static std::string current_filename_;

        Exception(const std::string & msg, const std::string & expected, const Lexer::Tokens::Token & token) {
            rawMessage_ = msg + ": " + token.dump();
            if (current_filename_ == "-") {
                context_ = "at line: " + std::to_string(token.line_number) +
                           ", column: " + std::to_string(token.column_number);
            } else {
                context_ = " in file \"" + current_filename_ + "\" at line: " + std::to_string(token.line_number) +
                           ", column: " + std::to_string(token.column_number);
            }
            if (expected.empty() == false) {
                rawMessage_ += " (expected: " + expected + ")";
            }
            formattedMessage_ = formatMessage();
        }

        Exception(const std::string & msg, const std::string & expected, int line, int col) {
            rawMessage_ = msg;
            if (expected.empty() == false) {
                rawMessage_ += " (expected: " + expected + ")";
            }
            context_ = " in file \"" + current_filename_ + "\" at line: " + std::to_string(line) +
                       ", column: " + std::to_string(col);
            formattedMessage_ = formatMessage();
        }

        std::string formatMessage() const override { return "[Syntax ERROR] >>" + context_ + " << : " + rawMessage_; }
    };

    void parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                     const std::string & filename);
    static const std::unordered_map<std::string, Lexer::Tokens::Type>              keywords;
    static const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> variable_types;

    // Helper method to parse a statement body enclosed in { }
    std::vector<std::unique_ptr<Interpreter::StatementNode>> parseStatementBody(const std::string & errorContext);

  private:
    std::vector<Lexer::Tokens::Token> tokens_;
    std::string_view                  input_str_view_;
    size_t                            current_token_index_;
    std::string                       current_filename_;

    // Validation functions
    void validateTokenStream();
    void validateParserState();

    // Token stream handling and error-reporting helper functions (unchanged)
    const Lexer::Tokens::Token & currentToken() const;

    // Look ahead in the token stream
    const Lexer::Tokens::Token & peekToken(size_t offset = 1) const;

    // Consume (advance past) the current token and return it
    Lexer::Tokens::Token consumeToken();

    // Check if current token type matches the expected type
    // If so, consume it and return true; otherwise return false
    bool match(Lexer::Tokens::Type expected_type);

    // Check if current token type and value match the expected ones
    // Only use value checking for operators and punctuation
    bool match(Lexer::Tokens::Type expected_type, const std::string & expected_value);

    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type);

    // Like expect, but also checks the token's value
    Lexer::Tokens::Token expect(Lexer::Tokens::Type expected_type, const std::string & expected_value);

    // Check if we've reached the end of relevant tokens (just before EOF)
    bool isAtEnd() const;

    [[noreturn]] void reportError(const std::string & message, const std::string & expected = "") {
        if (current_token_index_ < tokens_.size()) {
            throw Exception(message, expected, tokens_[current_token_index_]);
        }
        int line = tokens_.empty() ? 0 : tokens_.back().line_number;
        int col  = tokens_.empty() ? 0 : tokens_.back().column_number;
        throw Exception(message, expected, line, col);
    }

    [[noreturn]] static void reportError(const std::string & message, const Lexer::Tokens::Token & token,
                                         const std::string & expected = "") {
        throw Exception(message, expected, token);
    }

    // Main entry point for parsing top-level statements in a script
    void                                        parseTopLevelStatement();
    // Parse a statement node for use inside blocks (if, for, while, function bodies)
    std::unique_ptr<Interpreter::StatementNode> parseStatementNode();

    // Parse a top-level constant variable definition (e.g., const <type> $name = expr;)
    void                                        parseConstVariableDefinition();
    // Parse a top-level include
    void                                        parseIncludeStatement();
    // Parse a top-level variable definition (e.g., <type> $name = expr;)
    void                                        parseVariableDefinition();
    void                                        parseFunctionDefinition();
    // Parse a top-level function call statement (e.g., foo(arg1, arg2);)
    std::unique_ptr<Interpreter::StatementNode> parseCallStatement();
    // Parse a top-level assignment statement (variable or object member)
    void                                        parseAssignmentStatement();
    // Parse a top-level class definition: class Name { ... }
    void                                        parseClassDefinition();
    // Parse a return statement (e.g., return; or return expr;)
    void                                        parseReturnStatement();
    // Parse an if-else conditional statement (at top-level)
    void                                        parseIfStatement();
    // Parse a for-in loop over object members (at top-level)
    void                                        parseForStatement();
    // Parse a while loop statement
    void                                        parseWhileStatement();
    // Parse an empty statement (just a semicolon)
    void                                        parseEmptyStatement();
    // Parse an if-else conditional block and return a StatementNode (for nested blocks)
    std::unique_ptr<Interpreter::StatementNode> parseIfStatementNode();
    // Parse a for-in loop over object members and return a StatementNode (for nested blocks)
    std::unique_ptr<Interpreter::StatementNode> parseForStatementNode();
    // Parse a while loop over
    std::unique_ptr<Interpreter::StatementNode> parseWhileStatementNode();
    // Parse an assignment statement (variable, object member, 'this' member) and return its node
    // Used by both parseTopLevelStatement and parseStatementNode
    std::unique_ptr<Interpreter::StatementNode> parseAssignmentStatementNode();

    // Parse a return statement and return a StatementNode (for nested blocks)
    std::unique_ptr<Interpreter::StatementNode> parseReturnStatementNode();

    // NEW: Parse a variable definition within a block and return its node
    std::unique_ptr<Interpreter::StatementNode> parseVariableDefinitionNode();

    // --- Parsing helper functions ---

    // type : KEYWORD_STRING | KEYWORD_INT | KEYWORD_DOUBLE
    // Returns the corresponding Symbols::Variables::Type enum and consumes the token
    Symbols::Variables::Type parseType();
    Symbols::ValuePtr           parseValue(Symbols::Variables::Type expected_var_type);
    Symbols::ValuePtr      parseNumericLiteral(const std::string & value, bool is_negative, Symbols::Variables::Type type);
    /**
     * @brief Parse the body of a function or method, creating a new scope and recording its operations.
     * @param opening_brace_idx Index in tokens_ of the '{' token that opens the function body.
     * @param function_name    Name of the function being parsed.
     * @param return_type      Expected return type of the function.
     * @param params           Parameter list for the function.
     */
    void                parseFunctionBody(size_t opening_brace_idx, const std::string & function_name,
                                          Symbols::Variables::Type return_type, const Symbols::FunctionParameterInfo & params);
    ParsedExpressionPtr parseParsedExpression(const Symbols::Variables::Type & expected_var_type);

    // Helper to parse an identifier name, stripping leading '$' if present
    static std::string parseIdentifierName(const Lexer::Tokens::Token & token) {
        std::string name = token.value;
        if (!name.empty() && name[0] == '$') {
            return name.substr(1);
        }
        return name;
    }

    // Helper to parse this->$property access as a special case
    ParsedExpressionPtr parseThisPropertyAccess();

};  // class Parser

}  // namespace Parser

#endif  // PARSER_HPP
