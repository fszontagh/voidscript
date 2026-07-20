#ifndef CONTINUE_EXCEPTION_HPP
#define CONTINUE_EXCEPTION_HPP

#include "Interpreter/ControlFlowSignal.hpp"

namespace Interpreter {

// NOT a std::exception - see ControlFlowSignal. A loop catches this to skip the rest
// of the current iteration; nothing else may absorb it.
class ContinueException : public ControlFlowSignal {
  public:
    ContinueException() = default;
};

}  // namespace Interpreter

#endif  // CONTINUE_EXCEPTION_HPP
