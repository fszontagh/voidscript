#ifndef CONTINUE_NODE_HPP
#define CONTINUE_NODE_HPP

#include <string>
#include <memory>

#include "../../StatementNode.hpp" // Base class
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class (for Accept method)
#include "../../../Interpreter/ContinueException.hpp" // Added for ContinueException

// Forward declare Interpreter is no longer needed due to direct include of Interpreter.hpp

namespace Interpreter::Nodes::Statement {

class ContinueNode : public ::Interpreter::StatementNode {
public:
    ContinueNode(
        const std::string& file_name,
        int file_line,
        size_t line_column
    ) : ::Interpreter::StatementNode(file_name, file_line, line_column) {}

    // The Accept method is the equivalent of 'interpret' for the visitor pattern
    void Accept(::Interpreter::Interpreter& interpreter) const {
        this->interpret(interpreter);
    }

    // Implementation for the pure virtual toString() method
    std::string toString() const override {
        return "ContinueNode()";
    }

    // interpret() is pure virtual in StatementNode.
    void interpret(::Interpreter::Interpreter& interpreter) const override {
        // The 'interpreter' parameter is not used by this specific node's logic.
        throw ContinueException();
    }
};

} // namespace Interpreter::Nodes::Statement

#endif // CONTINUE_NODE_HPP
