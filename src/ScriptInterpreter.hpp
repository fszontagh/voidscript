#ifndef SSCRIPTINTERPRETER_HPP
#define SSCRIPTINTERPRETER_HPP
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseFunction.hpp"
#include "Token.hpp"
#include "Value.hpp"

using FunctionValidator = std::function<void(const std::vector<Token> &, size_t &)>;

class ScriptInterpreter {
  public:
    void registerFunction(const std::string & name, std::shared_ptr<BaseFunction> fn);
    void executeScript(const std::string & source, const std::string & filenaneame, bool debug = false);

    static void throwUnexpectedTokenError(const Token & token, const std::string & expected = "",
                                          const std::string & file = "", const int & line = 0) {
        const std::string error_content =
            "unexpected token: '" + token.lexeme + "' type: " + tokenTypeNames.at(token.type) +
            (expected.empty() ? "" : ", expected: '" + expected + "'") + " in file: " + token.file + ":" +
            std::to_string(token.lineNumber) + ":" + std::to_string(token.columnNumber);
#ifdef DEBUG_BUILD
        const std::string error_message = file + ":" + std::to_string(line) + "\n" + error_content;
#else
        const std::string& error_message = error_content;
#endif
        throw std::runtime_error(error_message);
    };

    static void throwUndefinedVariableError(const std::string & name, const Token & token, const std::string & file,
                                            const int & line = 0) {
        const std::string error_content = "undefined variable: '$" + name + "' in file: " + token.file + ":" +
                                          std::to_string(token.lineNumber) + ":" + std::to_string(token.columnNumber);
#ifdef DEBUG_BUILD
        const std::string error_message = file + ":" + std::to_string(line) + "\n" + error_content;
#else
        const std::string& error_message = error_content;
#endif
        throw std::runtime_error(error_message);
    }

    static void throwVariableTypeMissmatchError(const std::string & target_variable_name,
                                                const std::string & target_type,
                                                const std::string & source_variable_name,
                                                const std::string & source_type, const Token & token,
                                                const std::string & file = "", const int & line = 0) {
        std::string error_content =
            "variable type missmatch: '$" + target_variable_name + "' declared type: '" + target_type + "'";
        if (!source_variable_name.empty()) {
            error_content += " source variable: '" + source_variable_name + "'";
        }
        if (!source_type.empty()) {
            error_content += " assigned type: '" + source_type + "'";
        }

        error_content += " in file: " + token.file + ":" + std::to_string(token.lineNumber) + ":" +
                         std::to_string(token.columnNumber);
#ifdef DEBUG_BUILD
        const std::string error_message = file + ":" + std::to_string(line) + "\n" + error_content;
#else
        const std::string error_message = error_content;
#endif
        throw std::runtime_error(error_message);
    }

    static void throwVariableRedefinitionError(const std::string & name, const Token & token,
                                               const std::string & file = "", const int line = 0) {
        const std::string error_content = "variable alread defined: " + name + " in file: " + token.file + ":" +
                                          std::to_string(token.lineNumber) + ":" + std::to_string(token.columnNumber);
#ifdef DEBUG_BUILD
        const std::string error_message = file + ":" + std::to_string(line) + "\n" + error_content;
#else
        const std::string& error_message = error_content;
#endif
        throw std::runtime_error(error_message);
    }

  private:
    std::unordered_map<std::string, FunctionValidator>             functionValidators;
    std::unordered_map<std::string, std::shared_ptr<BaseFunction>> functionObjects;
    std::unordered_map<std::string, Value>                         variables;

    std::vector<Value> parseArguments(const std::vector<Token> & tokens, std::size_t & current_index) const;
    Value              evaluateExpression(const Token & token) const;
    // type handlers
    void expectSemicolon(const std::vector<Token> & tokens, std::size_t & i, const std::string & message) const;
    void handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i);
    void handleVariableReference(const std::vector<Token> & tokens, std::size_t & i);
    void handleComment(std::size_t & i);
    void handleSemicolon(std::size_t & i);
    void handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i);
    void handleNumberDeclaration(const std::vector<Token> & tokens, std::size_t & i, TokenType type);
};

#endif  // SSCRIPTINTERPRETER_HPP
