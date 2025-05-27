#ifndef BREAK_NODE_HPP
#define BREAK_NODE_HPP

#include <string>
#include <memory>

#include "../../StatementNode.hpp" // Base class
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class (for Accept method)

// Forward declare Interpreter to avoid circular dependency if Interpreter.hpp includes this node
// class Interpreter::Interpreter; // Usually handled by including Interpreter.hpp

namespace Interpreter::Nodes::Statement {

class BreakNode : public ::Interpreter::StatementNode {
public:
    BreakNode(
        const std::string& file_name,
        int file_line,
        size_t line_column
    ) : ::Interpreter::StatementNode(file_name, file_line, line_column) {}

    // The Accept method is the equivalent of 'interpret' for the visitor pattern
    void Accept(::Interpreter::Interpreter& interpreter) const;

    // Implementation for the pure virtual toString() method
    std::string toString() const override;

    // interpret() is pure virtual in StatementNode.
    void interpret(::Interpreter::Interpreter& interpreter) const override;
};

} // namespace Interpreter::Nodes::Statement

#endif // BREAK_NODE_HPP
