#ifndef PRINTFUNCTION_HPP
#define PRINTFUNCTION_HPP

#include <iostream>

#include "BaseFunction.hpp"
#include "ScriptExceptionMacros.h"
#include "Token.hpp"
#include "Value.hpp"

class PrintFunction : public BaseFunction {
  private:
    const std::string name       = "print";
    bool              addNewLine = false;
  public:
    PrintFunction() : BaseFunction(name) {}

    void validateArgs(const std::vector<Token> &                     args,
                      const std::unordered_map<std::string, Value> & variables) override {
        if (args.size() == 0) {
            THROW_UNEXPECTED_TOKEN_ERROR(args[0], "at least one argument");
        }

        for (const auto & arg : args) {
            if (arg.type == TokenType::Variable) {
                if (!variables.contains(arg.lexeme)) {
                    THROW_UNDEFINED_VARIABLE_ERROR(arg.lexeme, arg);
                }
            }
        }
        if (args.end()->variableType == Variables::Type::VT_INT || args.end()->type == TokenType::IntLiteral) {
            this->addNewLine = true;
        }
    }

    Value call(const std::vector<Value> & args, bool debug = false) const override {
        for (const auto & arg : args) {
            std::cout << arg.ToString(); // todo: add endline if the last parameter is bool
        }
        return Value();
    }
};

#endif  // PRINTFUNCTION_HPP
