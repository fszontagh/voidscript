#include "SwitchStatementNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

namespace Interpreter::Nodes::Statement {

void SwitchStatementNode::Accept(class Interpreter::Interpreter& interpreter) const {
    interpreter.Visit(*this);
}

void SwitchStatementNode::interpret(::Interpreter::Interpreter& interpreter) const {
    this->Accept(interpreter);
    // Or directly: interpreter.Visit(*this);
    // Calling Accept is slightly cleaner if Accept might do more in the future.
}

} // namespace Interpreter::Nodes::Statement
