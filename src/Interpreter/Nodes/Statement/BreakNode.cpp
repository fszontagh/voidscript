#include "BreakNode.hpp"
// Method definitions are now inline in BreakNode.hpp.
// Includes like Interpreter.hpp and BreakException.hpp are now primarily needed in the header.
// Retaining some includes here in case other parts of the build system might have (incorrectly)
// relied on this .cpp file providing them transitively. For a pure header-only approach,
// this .cpp file would ideally be almost empty or just #include "BreakNode.hpp".

#include "../../../Interpreter/Interpreter.hpp" // Retained for now
#include "Interpreter/BreakException.hpp" // Retained for now


namespace Interpreter::Nodes::Statement {

// Definitions for BreakNode::Accept, BreakNode::interpret, and BreakNode::toString
// are now inline in BreakNode.hpp.

} // namespace Interpreter::Nodes::Statement
