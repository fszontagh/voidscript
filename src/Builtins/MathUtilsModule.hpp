#ifndef MATH_UTILS_MODULE_HPP
#define MATH_UTILS_MODULE_HPP

#include <stdexcept>
#include <vector>

#include "ScriptExceptionMacros.h"
#include "Value.hpp"

class MathUtils {
  public:
    static Value multiply(const std::vector<Value> & args) {
        if (args.size() != 2) {
            throw std::runtime_error("multiply expects two arguments.");
        }
        if (args[0].type == Variables::Type::VT_INT && args[1].type == Variables::Type::VT_INT) {
            int  left   = args[0].ToInt();
            int  right  = args[1].ToInt();
            auto result = Value();
            result.data = left * right;
            result.type = Variables::Type::VT_INT;
            return result;
            //return Value::fromInt(left * right);
        }
        THROW_INVALID_FUNCTION_ARGUMENT_ERROR("multiply", args[0].TypeToString(), args[0].GetToken());
    };
};
#endif
