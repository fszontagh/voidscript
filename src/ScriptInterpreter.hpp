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
using VariableContext = std::unordered_map<std::string, Value>;

class ScriptInterpreter {
  public:
    void registerModule(const std::string & name, std::shared_ptr<BaseFunction> fn);
    void executeScript(const std::string & source, const std::string & filenaneame, bool ignore_tags = false);

  private:
    std::unordered_map<std::string, FunctionValidator>             functionValidators;
    std::unordered_map<std::string, std::shared_ptr<BaseFunction>> functionObjects;
    // store the current script's variables
    std::unordered_map<std::string, VariableContext>               variables;
    // store the current script's function arguments
    std::unordered_map<std::string, std::vector<Value>>            functionParameters;
    std::unordered_map<std::string, std::string>                   functionBodies;
    std::string                                                    filename;

    [[nodiscard]] std::vector<Value> parseFunctionArguments(const std::vector<Token> & tokens,
                                                            std::size_t &              index) const;
    [[nodiscard]] Value              evaluateExpression(const Token & token) const;

    // type handlers
    void setVariable(const std::string & name, const Value & value, const std::string & context = "default") {
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

    [[nodiscard]] Value getVariable(const Token & token, const std::string & context = "default") const {
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            if (it->first == context) {
                const auto & innerMap = it->second.find(token.lexeme);
                if (innerMap != it->second.end()) {
                    return it->second.at(token.lexeme);
                }
            }
        }
        THROW_UNDEFINED_VARIABLE_ERROR(token.lexeme, token);
    };

    void handleFunctionCall(const std::vector<Token> & tokens, std::size_t & i);
    void handleVariableReference(const std::vector<Token> & tokens, std::size_t & i);
    void handleComment(std::size_t & i);
    void handleSemicolon(std::size_t & i);
    void handleStringDeclaration(const std::vector<Token> & tokens, std::size_t & i);
    void handleBooleanDeclaration(const std::vector<Token> & tokens, std::size_t & i);
    void handleNumberDeclaration(const std::vector<Token> & tokens, std::size_t & i, TokenType type);
    void handleFunctionDeclaration(const std::vector<Token> & tokens, std::size_t & i);
};

#endif  // SSCRIPTINTERPRETER_HPP
