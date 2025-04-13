#ifndef SLEEPFUNCTION_HPP
#define SLEEPFUNCTION_HPP

#include <thread>

#include "BaseFunction.hpp"

class SleepFunction : public BaseFunction {
  public:
    SleepFunction() : BaseFunction("sleep") {}

    void validateArgs(const std::vector<Token> &                     args,
                      const std::unordered_map<std::string, Value> & variables) override {
        if (args.size() != 1) {
            throw std::runtime_error("sleep() requires exactly one argument");
        }

        const Token & arg = args[0];

        if (arg.type == TokenType::IntLiteral) {
            return;
        }

        if (arg.type == TokenType::Variable) {
            const auto & value = variables.at(arg.lexeme);
            if (value.type != Variables::Type::VT_INT) {
                THROW_VARIABLE_TYPE_MISSMATCH_ERROR(arg.lexeme, Variables::TypeToString(Variables::Type::VT_INT), "",
                                                    Variables::TypeToString(value.type), arg);
            }
            return;
        }

        THROW_UNEXPECTED_TOKEN_ERROR(arg, "int literal or variable");
    }


    Value call(const std::vector<Value> & args, bool debug = false) const override {
        std::this_thread::sleep_for(std::chrono::seconds(args[0].ToInt()));
        return Value();
    }
};

#endif  // SLEEPFUNCTION_HPP
