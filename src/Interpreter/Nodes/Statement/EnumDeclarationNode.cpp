#include "EnumDeclarationNode.hpp" 
// Other includes like Interpreter.hpp and EnumSymbol.hpp are now only needed in the header.
// However, keeping Interpreter.hpp might be necessary if other cpp files include this .cpp file expecting it to provide that.
// For a true header-only approach, this .cpp file would ideally be empty or just contain the #include "EnumDeclarationNode.hpp".
// For now, just removing the method bodies.
#include "../../../Interpreter/Interpreter.hpp" // Retain for now, might be part of a precompiled header or expected by other includes.
#include "Symbols/EnumSymbol.hpp" // Retain for now.

namespace Interpreter::Nodes::Statement {

// Method definitions are now inline in EnumDeclarationNode.hpp
// Accept() is defined in the header.
// interpret() is defined in the header.

} // namespace Interpreter::Nodes::Statement
