#ifndef BREAK_EXCEPTION_HPP
#define BREAK_EXCEPTION_HPP

#include <stdexcept> // For std::runtime_error

namespace Interpreter {

class BreakException : public std::runtime_error {
public:
    BreakException() : std::runtime_error("Break statement encountered") {}
    // It might be useful to allow a message, though not strictly needed for break
    // explicit BreakException(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace Interpreter

#endif // BREAK_EXCEPTION_HPP
