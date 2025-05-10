// ReturnException.hpp
#ifndef INTERPRETER_RETURN_EXCEPTION_HPP
#define INTERPRETER_RETURN_EXCEPTION_HPP

#include <stdexcept>
#include <string>

#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Exception thrown when a return statement is executed.
 * 
 * This exception is used to immediately exit from a function or method
 * and return control to the caller with the specified return value.
 */
class ReturnException : public std::exception {
    Symbols::Value value_;
    std::string    message_;

  public:
    /**
     * @brief Construct a new Return Exception object with a return value
     * @param value The value to return from the function
     */
    explicit ReturnException(Symbols::Value value) : value_(std::move(value)) {
        message_ = "Return with value of type: " + Symbols::Variables::TypeToString(value_.getType());
    }

    /**
     * @brief Construct a new Return Exception object with no return value
     */
    ReturnException() : value_(Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE)) {
        message_ = "Return with no value";
    }

    /**
     * @brief Get the return value
     * @return The return value
     */
    const Symbols::Value & value() const { return value_; }

    /**
     * @brief Get the error message
     * @return The error message
     */
    const char * what() const noexcept override { return message_.c_str(); }
};
} // namespace Interpreter
#endif // INTERPRETER_RETURN_EXCEPTION_HPP