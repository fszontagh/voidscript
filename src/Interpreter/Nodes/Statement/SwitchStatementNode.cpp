#include "SwitchStatementNode.hpp"
// Method definitions are now inline in SwitchStatementNode.hpp.
// Includes like Interpreter.hpp, ExpressionNode.hpp, BreakException.hpp are now primarily needed in the header.
// Retaining some includes here in case other parts of the build system might have (incorrectly)
// relied on this .cpp file providing them transitively. For a pure header-only approach,
// this .cpp file would ideally be almost empty or just #include "SwitchStatementNode.hpp".

#include "../../../Interpreter/Interpreter.hpp" // Retained for now
#include "../../ExpressionNode.hpp" // Retained for now
#include "Symbols/SymbolContainer.hpp" // Retained for now
#include "Interpreter/BreakException.hpp" // Retained for now


namespace Interpreter::Nodes::Statement {

// Definitions for SwitchStatementNode::Accept and SwitchStatementNode::interpret
// are now inline in SwitchStatementNode.hpp.

} // namespace Interpreter::Nodes::Statement
