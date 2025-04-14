#ifndef SSCRIPTINTERPRETER_HPP
#define SSCRIPTINTERPRETER_HPP
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseFunction.hpp"
#include "Token.hpp"
#include "Value.hpp"

using FunctionValidator =
    std::function<void(const std::vector<Token> &, size_t &, const std::unordered_map<std::string, Value> &)>;
using VariableContext = std::map<std::string, Value>;

class ScriptInterpreter {
  public:
    void registerModule(const std::string & name, std::shared_ptr<BaseFunction> fn);
    void executeScript(const std::string & source, const std::string & filename,
                       const std::string & _namespace = "DEFAULT", bool ignore_tags = false);

  private:
    std::unordered_map<std::string, FunctionValidator>             functionValidators;
    std::unordered_map<std::string, std::shared_ptr<BaseFunction>> functionObjects;
    // store the current script's variables
    std::unordered_map<std::string, VariableContext>               variables;
    // store the current script's function arguments
    std::unordered_map<std::string, std::vector<Value>>            functionParameters;
    std::unordered_map<std::string, std::string>                   functionBodies;
    std::string                                                    filename;
    std::string                                                    source;
    std::string                                                    contextPrefix;

    [[nodiscard]] std::vector<Value> parseFunctionArguments(const std::vector<Token> & tokens,
                                                            std::size_t &              index) const;
    [[nodiscard]] Value              evaluateExpression(const Token & token) const;

    // type handlers
    void setVariable(const std::string & name, Value & value, const std::string & context = "default",
                     bool exception_if_exists = false, bool exception_if_not_exists = false) {
        if (exception_if_exists && variables[context].find(name) != variables[context].end()) {
            THROW_VARIABLE_REDEFINITION_ERROR(name, value.token);
        }
        if (exception_if_not_exists && variables[context].find(name) == variables[context].end()) {
            THROW_UNDEFINED_VARIABLE_ERROR(name, value.token);
        }
        value.name                     = name;
        value.context                  = context;
        this->variables[context][name] = value;
    }

    [[nodiscard]] Value getVariable(const std::string & name, const std::string & context = "default") const {
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            if (it->first == context) {
                const auto & innerMap = it->second.find(name);
                if (innerMap != it->second.end()) {
                    return it->second.at(name);
                }
            }
        }
        throw std::runtime_error("Variable not found: " + name);
    };

    [[nodiscard]] Value getVariable(const Token & token, const std::string & context = "default",
                                    const std::string & file = __FILE__, const int & line = __LINE__) const {
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            if (it->first == context) {
                const auto & innerMap = it->second.find(token.lexeme);
                if (innerMap != it->second.end()) {
                    return it->second.at(token.lexeme);
                }
            }
        }
        THROW_UNDEFINED_VARIABLE_ERROR_HELPER(token.lexeme, token, file, line);
    };

    [[nodiscard]] std::map<std::string, Value> getcontextVariables(const std::string & context) const {
        auto it = variables.find(context);
        if (it != variables.end()) {
            return it->second;
        }
        throw std::runtime_error("Context not found: " + context);
    }

    /**
         * Checks if a variable exists within the specified context.
         *
         * @param name The name of the variable to check.
         * @param context The context in which to search for the variable (defaults to "default").
         * @param file The source file where the check is performed (for error reporting).
         * @param line The line number where the check is performed (for error reporting).
         * @return True if the variable exists in the specified context.
         * @throws Throws an undefined variable error if the variable is not found.
         */
    [[nodiscard]] bool variableExists(const std::string & name, const std::string & context = "default",
                                      const std::string & file = __FILE__, const int & line = __LINE__) const {
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            if (it->first == context) {
                const auto & innerMap = it->second.find(name);
                if (innerMap != it->second.end()) {
                    return true;
                }
            }
        }
        THROW_UNDEFINED_VARIABLE_ERROR_HELPER(name, Token(TokenType::Variable, name, name, 0, 0), file, line);
    }

    /**
     * Checks if a variable exists within the specified context using a Token.
     *
     * @param token The token representing the variable to check.
     * @param context The context in which to search for the variable (defaults to "default").
     * @param file The source file where the check is performed (for error reporting).
     * @param line The line number where the check is performed (for error reporting).
     * @return True if the variable exists in the specified context.
     * @throws Throws an undefined variable error if the variable is not found.
     */
    [[nodiscard]] bool variableExists(const Token & token, const std::string & context = "default",
                                      const std::string & file = __FILE__, const int & line = __LINE__) const {
        return this->variableExists(token.lexeme, context, file, line);
    }

    void handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i);
    void handleVariableReference(const std::vector<Token> & tokens, std::size_t & i);

    static void handleComment(std::size_t & i) { i++; }

    static void handleSemicolon(std::size_t & i) { i++; };

    void handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i);
    void handleBooleanDeclaration(const std::vector<Token> & tokens, std::size_t & i);
    void handleNumberDeclaration(const std::vector<Token> & tokens, std::size_t & i, TokenType type);
    void handleFunctionDeclaration(const std::vector<Token> & tokens, std::size_t & i);

    std::string getContextName(const std::string & suffix) const { return this->filename + "::" + suffix; }
};

#endif  // SSCRIPTINTERPRETER_HPP
