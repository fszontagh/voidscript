#ifndef ENUM_DECLARATION_NODE_HPP
#define ENUM_DECLARATION_NODE_HPP

#include <string>
#include <vector>
#include <utility> // For std::pair
#include <optional> // For std::optional
#include <memory> // For std::unique_ptr, though not directly used for members here, good practice

#include "../../StatementNode.hpp" // Base class
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class, Exception, SymbolContainer
#include "Symbols/EnumSymbol.hpp" // Corrected path

// Forward declare Interpreter is no longer needed due to direct include of Interpreter.hpp

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

    void Accept(::Interpreter::Interpreter& interpreter) const { // Removed 'class' from param, added ::
        this->interpret(interpreter);
    }

    // interpret() is pure virtual in StatementNode.
    void interpret(::Interpreter::Interpreter& interpreter) const override {
        // 'interpreter' parameter is available for context if needed.
        std::string context_str = this->filename_ + ":" + std::to_string(this->line_) + ":" + std::to_string(this->column_);
        try {
            // Ensure Symbols::EnumSymbol and Symbols::SymbolContainer are accessible.
            // Interpreter.hpp (already included) brings SymbolContainer.hpp.
            // EnumSymbol.hpp is added above.
            auto enumSymbol = std::make_shared<::Symbols::EnumSymbol>(
                this->enumName,
                this->enumerators,
                context_str 
            );
            ::Symbols::SymbolContainer::instance()->add(enumSymbol);
        } catch (const std::runtime_error& e) {
            // Interpreter::Exception should be accessible from Interpreter.hpp.
            throw ::Interpreter::Exception(e.what(), this->filename_, this->line_, this->column_);
        }
    }

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
