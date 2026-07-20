#ifndef BREAK_EXCEPTION_HPP
#define BREAK_EXCEPTION_HPP

#include "Interpreter/ControlFlowSignal.hpp"

namespace Interpreter {

// NOT a std::exception - see ControlFlowSignal. This used to derive from
// std::runtime_error, which let a generic catch swallow `break` inside a try.
class BreakException : public ControlFlowSignal {
public:
    BreakException() = default;
    // It might be useful to allow a message, though not strictly needed for break
    // explicit BreakException(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace Interpreter

#endif // BREAK_EXCEPTION_HPP
