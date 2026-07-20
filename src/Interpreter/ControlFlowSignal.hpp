#ifndef INTERPRETER_CONTROL_FLOW_SIGNAL_HPP
#define INTERPRETER_CONTROL_FLOW_SIGNAL_HPP

namespace Interpreter {

/**
 * @brief Base for exceptions that carry control flow rather than report an error.
 *
 * Deliberately does NOT derive from std::exception. `break`, `continue` and `return`
 * are unwound with exceptions, but they are not failures, and a generic
 * `catch (const std::exception &)` anywhere in the interpreter must not be able to
 * absorb them.
 *
 * This is not a style preference. BreakException used to derive from
 * std::runtime_error, so the generic handler in TryStatementNode swallowed a `break`
 * inside a `try` and turned the enclosing loop infinite. Making these types
 * un-catchable by the generic handler removes that whole class of bug: a new catch
 * site cannot get it wrong by omission, because the compiler will not let it.
 *
 * Anything that must observe control flow catches ControlFlowSignal, or one of its
 * derived types, explicitly.
 */
class ControlFlowSignal {
  public:
    virtual ~ControlFlowSignal() = default;
};

}  // namespace Interpreter

#endif  // INTERPRETER_CONTROL_FLOW_SIGNAL_HPP
