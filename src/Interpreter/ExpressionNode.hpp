#ifndef INTERPRETER_FUNCTION_EXECUTOR_HPP
#define INTERPRETER_FUNCTION_EXECUTOR_HPP

#include "Symbols/Value.hpp"

namespace Interpreter {
struct ExpressionNode {
    std::string filename;
    int         line;
    size_t      column;
    virtual ~ExpressionNode()                                   = default;
    virtual Symbols::ValuePtr evaluate(class Interpreter & interpreter, std::string filename = "", int line = 0,
                                       size_t column = 0) const = 0;
    virtual std::string       toString() const                  = 0;
};

}  // namespace Interpreter
#endif  // INTERPRETER_FUNCTION_EXECUTOR_HPP
