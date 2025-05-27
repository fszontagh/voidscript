#include "BreakNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

namespace Interpreter::Nodes::Statement {

void BreakNode::Accept(class Interpreter::Interpreter& interpreter) const {
    interpreter.Visit(*this);
}

std::string BreakNode::toString() const {
    return "BreakNode()";
}

void BreakNode::interpret(class Interpreter::Interpreter& interpreter) const {
    // This method might not be called if the system exclusively uses Accept.
    // If it is called, it should delegate to the visitor pattern.
    Accept(interpreter);
}

} // namespace Interpreter::Nodes::Statement
