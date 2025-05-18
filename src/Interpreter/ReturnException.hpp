// ReturnException.hpp
#ifndef INTERPRETER_RETURN_EXCEPTION_HPP
#define INTERPRETER_RETURN_EXCEPTION_HPP

#include <utility>

#include "Symbols/Value.hpp"

namespace Interpreter {
/**
 * @brief Exception used to unwind the call stack when a return statement is executed.
 */
class ReturnException {
  public:
    explicit ReturnException(Symbols::Value::ValuePtr value) : value_(std::move(value)) {}

    const Symbols::Value::ValuePtr & value() const { return value_; }
  private:
    Symbols::Value::ValuePtr value_;
};
}  // namespace Interpreter
#endif  // INTERPRETER_RETURN_EXCEPTION_HPP
