#ifndef ENUM_DECLARATION_NODE_HPP
#define ENUM_DECLARATION_NODE_HPP

#include <string>
#include <vector>
#include <utility> // For std::pair
#include <optional> // For std::optional
#include <memory> // For std::unique_ptr, though not directly used for members here, good practice

#include "../../StatementNode.hpp" // Base class
#include "../../../Interpreter/Interpreter.hpp" // Forward declaration for Interpreter usually handled by including this

// Forward declare Interpreter to avoid circular dependency if Interpreter.hpp includes this node
// class Interpreter::Interpreter; // This is not standard, better to include Interpreter.hpp

namespace Interpreter::Nodes::Statement {

class EnumDeclarationNode : public ::Interpreter::StatementNode {
public:
    std::string enumName;
    std::vector<std::pair<std::string, std::optional<int>>> enumerators;

    EnumDeclarationNode(
        const std::string& file_name,
        int file_line,
        size_t line_column,
        std::string name,
        std::vector<std::pair<std::string, std::optional<int>>> enums
    ) : ::Interpreter::StatementNode(file_name, file_line, line_column),
        enumName(std::move(name)),
        enumerators(std::move(enums)) {}

    void Accept(class Interpreter::Interpreter& interpreter) const;

    // It's good practice to have a toString for debugging, though not strictly required by the task
    std::string toString() const override {
        std::string str = "EnumDeclarationNode(\n";
        str += "  EnumName: " + enumName + ",\n";
        str += "  Enumerators: [\n";
        for (const auto& enumerator : enumerators) {
            str += "    { Name: " + enumerator.first;
            if (enumerator.second.has_value()) {
                str += ", Value: " + std::to_string(enumerator.second.value());
            }
            str += " },\n";
        }
        str += "  ]\n";
        str += ")";
        return str;
    }
};

} // namespace Interpreter::Nodes::Statement

#endif // ENUM_DECLARATION_NODE_HPP
