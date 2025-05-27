#include "SwitchStatementNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

namespace Interpreter::Nodes::Statement {

void SwitchStatementNode::Accept(class Interpreter::Interpreter& interpreter) const {
    interpreter.Visit(*this);
}

} // namespace Interpreter::Nodes::Statement
