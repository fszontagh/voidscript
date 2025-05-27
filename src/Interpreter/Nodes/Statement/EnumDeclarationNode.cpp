#include "EnumDeclarationNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method
#include "Symbols/EnumSymbol.hpp" // Corrected path

namespace Interpreter::Nodes::Statement {

void EnumDeclarationNode::Accept(class Interpreter::Interpreter& interpreter) const {
    // Body left empty as per refactoring instructions.
}

void EnumDeclarationNode::interpret(::Interpreter::Interpreter& interpreter) const {
    // 'interpreter' parameter is available for context if needed by SymbolContainer or future extensions.
    // Members like 'this->filename_', 'this->enumName' are accessed directly.

    // std::string current_scope_name = Symbols::SymbolContainer::instance()->currentScopeName(); 
    // current_scope_name is not used in the original logic after removing debug log, so commenting out.

    std::string context_str = this->filename_ + ":" + std::to_string(this->line_) + ":" + std::to_string(this->column_);

    try {
        auto enumSymbol = std::make_shared<Symbols::EnumSymbol>(
            this->enumName,
            this->enumerators,
            context_str 
        );
        Symbols::SymbolContainer::instance()->add(enumSymbol);

        // Original debug logging that depended on a private member of Interpreter ('interpreter.debug_')
        // has been removed. If logging is needed here, it would require a different mechanism.

    } catch (const std::runtime_error& e) {
        // Interpreter::Exception should be accessible via the include of "Interpreter.hpp".
        // Use 'this->filename_', 'this->line_', 'this->column_' for error context.
        throw ::Interpreter::Exception(e.what(), this->filename_, this->line_, this->column_);
    }
}

} // namespace Interpreter::Nodes::Statement
