#ifndef INTERPRETER_FUNCTION_EXECUTOR_HPP
#define INTERPRETER_FUNCTION_EXECUTOR_HPP

#include "Symbols/Value.hpp"

namespace Interpreter {
struct ExpressionNode {
    // Must be initialised. Subclasses that do not carry a source location left these as
    // stack garbage, which the error formatter then printed verbatim - producing
    // "at line: 1942890312", different on every run. Zero is the sentinel every caller
    // already tests for before falling back to the enclosing statement's location.
    std::string filename;
    int         line   = 0;
    size_t      column = 0;
    virtual ~ExpressionNode()                                   = default;
    virtual Symbols::ValuePtr evaluate(class Interpreter & interpreter, std::string filename = "", int line = 0,
                                       size_t column = 0) const = 0;
    virtual std::string       toString() const                  = 0;
};

}  // namespace Interpreter
#endif  // INTERPRETER_FUNCTION_EXECUTOR_HPP
