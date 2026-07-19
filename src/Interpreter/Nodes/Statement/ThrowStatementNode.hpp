#ifndef INTERPRETER_THROW_STATEMENT_NODE_HPP
#define INTERPRETER_THROW_STATEMENT_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Interpreter/ThrowException.hpp"

namespace Interpreter {

/** @brief `throw expr;` - raises a value catchable by an enclosing try/catch. */
class ThrowStatementNode : public StatementNode {
  private:
    std::unique_ptr<ExpressionNode> expression_;

  public:
    ThrowStatementNode(std::unique_ptr<ExpressionNode> expression, const std::string & file, int line, size_t column) :
        StatementNode(file, line, column),
        expression_(std::move(expression)) {}

    void interpret(Interpreter & interpreter) const override {
        throw ThrowException(expression_->evaluate(interpreter, filename_, line_, column_), filename_, line_, column_);
    }

    std::string toString() const override { return "ThrowStatementNode{}"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_THROW_STATEMENT_NODE_HPP
