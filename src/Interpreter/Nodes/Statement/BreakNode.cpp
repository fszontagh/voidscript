#include "BreakNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

#include "Interpreter/BreakException.hpp" // For ::Interpreter::BreakException

namespace Interpreter::Nodes::Statement {

void BreakNode::Accept(class Interpreter::Interpreter& interpreter) const {
    this->interpret(interpreter);
}

std::string BreakNode::toString() const {
    return "BreakNode()";
}

void BreakNode::interpret(class Interpreter::Interpreter& interpreter) const {
    // The 'interpreter' parameter is not used by this specific node's logic.
    // The original debug log that used node.filename_ etc. and interpreter.debug_ is removed.
    throw ::Interpreter::BreakException(); 
}

} // namespace Interpreter::Nodes::Statement
