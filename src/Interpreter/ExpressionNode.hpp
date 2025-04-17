#ifndef INTERPRETER_FUNCTION_EXECUTOR_HPP
#define INTERPRETER_FUNCTION_EXECUTOR_HPP

#include "Symbols/Value.hpp"

namespace Interpreter {
struct ExpressionNode {
    virtual ~ExpressionNode()                                              = default;
    virtual Symbols::Value evaluate(class Interpreter & interpreter) const = 0;
    virtual std::string    toString() const                                = 0;
};

}  // namespace Interpreter
#endif  // INTERPRETER_FUNCTION_EXECUTOR_HPP
