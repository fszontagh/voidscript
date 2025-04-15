#ifndef INTERPRETER_FUNCTION_EXECUTOR_HPP
#define INTERPRETER_FUNCTION_EXECUTOR_HPP

namespace Interpreter {
struct StatementNode {
    virtual ~StatementNode()                                      = default;
    virtual void interpret(class Interpreter & interpreter) const = 0;
};

// Kifejezés (csak int literál most)
struct ExpressionNode {
    virtual ~ExpressionNode()                                   = default;
    virtual int evaluate(class Interpreter & interpreter) const = 0;
};

}  // namespace Interpreter
#endif  // INTERPRETER_FUNCTION_EXECUTOR_HPP
