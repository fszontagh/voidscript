#include "EnumDeclarationNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

namespace Interpreter::Nodes::Statement {

void EnumDeclarationNode::Accept(class Interpreter::Interpreter& interpreter) const {
    interpreter.Visit(*this);
}

void EnumDeclarationNode::interpret(::Interpreter::Interpreter& interpreter) const {
    this->Accept(interpreter);
    // Or directly: interpreter.Visit(*this);
    // Calling Accept is slightly cleaner if Accept might do more in the future.
}

} // namespace Interpreter::Nodes::Statement
