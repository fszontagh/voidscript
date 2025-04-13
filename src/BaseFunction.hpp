#ifndef SCRIPT_FUNCTION_HPP
#define SCRIPT_FUNCTION_HPP

#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ScriptExceptionMacros.h"
#include "ScriptInterpreterHelpers.hpp"
#include "Token.hpp"
#include "Value.hpp"

class ScriptInterpreter;

using CallbackFunction = std::function<Value(const std::vector<Value> &)>;
using CallBackStorage  = std::unordered_map<std::string, CallbackFunction>;

class BaseFunction {
  protected:
    std::string     name;
    CallBackStorage functionMap;


  public:
    BaseFunction(const std::string & functionName) : name(functionName) {}

    virtual void addFunction(const std::string & name, std::function<Value(const std::vector<Value> &)> callback) {
        functionMap[name] = std::move(callback);
    }

    virtual void validate(const std::vector<Token> & tokens, size_t & i,
                          const std::unordered_map<std::string, Value> & variables) {
        size_t index = i;

        if (tokens[index].type != TokenType::Identifier) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "identifier");
        }
        index++;  // skip function name

        if (tokens[index].type != TokenType::LeftParenthesis) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], "(");
        }
        index++;  // skip '('

        std::vector<Token> args;
        while (tokens[index].type != TokenType::RightParenthesis) {
            if (tokens[index].type == TokenType::Comma) {
                index++;
                continue;
            }
            if (tokens[index].type == TokenType::Variable && !variables.contains(tokens[index].lexeme)) {
                THROW_UNDEFINED_VARIABLE_ERROR(tokens[index].lexeme, tokens[index]);
            }
            args.push_back(tokens[index]);
            index++;
        }

        index++;  // skip ')'

        if (tokens[index].type != TokenType::Semicolon) {
            THROW_UNEXPECTED_TOKEN_ERROR(tokens[index], ";");
        }

        this->validateArgs(args, variables);
        ScriptInterpreterHelpers::expectSemicolon(tokens, index, "function call");
    }

    virtual void validateArgs(const std::vector<Token> &                     args,
                              const std::unordered_map<std::string, Value> & variables) = 0;

    virtual Value call(const std::vector<Value> & args, bool debug = false) const = 0;

    template <typename FuncClass> void registerFunctionTo(ScriptInterpreter & interp) { FuncClass::registerTo(interp); }
};

#endif  // SCRIPT_FUNCTION_HPP
