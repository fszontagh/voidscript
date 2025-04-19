// ReturnException.hpp
#ifndef INTERPRETER_RETURN_EXCEPTION_HPP
#define INTERPRETER_RETURN_EXCEPTION_HPP

#include "Symbols/Value.hpp"

namespace Interpreter {
/**
 * @brief Exception used to unwind the call stack when a return statement is executed.
 */
class ReturnException {
  public:
    explicit ReturnException(const Symbols::Value &value) : value_(value) {}
    const Symbols::Value &value() const { return value_; }
  private:
    Symbols::Value value_;
};
} // namespace Interpreter
#endif // INTERPRETER_RETURN_EXCEPTION_HPP