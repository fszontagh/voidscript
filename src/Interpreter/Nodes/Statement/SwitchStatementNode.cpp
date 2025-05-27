#include "SwitchStatementNode.hpp"
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class and its Visit method

#include "../../ExpressionNode.hpp" // Corrected path
#include "Symbols/SymbolContainer.hpp" // For ValuePtr, Variables::Type
#include "Interpreter/BreakException.hpp" // For BreakException

namespace Interpreter::Nodes::Statement {

void SwitchStatementNode::Accept(class Interpreter::Interpreter& interpreter) const {
    this->interpret(interpreter);
}

void SwitchStatementNode::interpret(::Interpreter::Interpreter& interpreter) const {
    // Use 'this->filename_', 'this->line_', 'this->column_' for node's own location.
    // Use 'interpreter' when calling methods on other objects that require an Interpreter reference (e.g. expression->evaluate).

    Symbols::ValuePtr switch_value = this->switchExpression->evaluate(
        interpreter, this->filename_, this->line_, this->column_
    );
    
    if (!switch_value || switch_value->isNULL() || switch_value->getType() != Symbols::Variables::Type::INTEGER) {
        const auto& expr_node = *this->switchExpression;
        // Prefer using expr_node's location if available, otherwise this node's location.
        throw ::Interpreter::Exception("Switch expression must evaluate to a non-null integer type.", 
                                     expr_node.filename.empty() ? this->filename_ : expr_node.filename, 
                                     expr_node.line == 0 ? this->line_ : expr_node.line, 
                                     expr_node.column == 0 ? this->column_ : expr_node.column);
    }

    bool matched_block_found = false;
    bool break_executed = false;

    for (const auto& case_block : this->caseBlocks) {
        if (break_executed) {
            break; 
        }

        if (!matched_block_found) {
            const auto& case_expr_node_ref = *case_block.expression;
            Symbols::ValuePtr case_expr_value = case_block.expression->evaluate(
                interpreter, 
                case_expr_node_ref.filename.empty() ? this->filename_ : case_expr_node_ref.filename, 
                case_expr_node_ref.line == 0 ? this->line_ : case_expr_node_ref.line, 
                case_expr_node_ref.column == 0 ? this->column_ : case_expr_node_ref.column
            );
            
            if (!case_expr_value || case_expr_value->isNULL() || case_expr_value->getType() != Symbols::Variables::Type::INTEGER) {
                 throw ::Interpreter::Exception("Case expression must evaluate to a non-null integer type.", 
                                     case_expr_node_ref.filename.empty() ? this->filename_ : case_expr_node_ref.filename, 
                                     case_expr_node_ref.line == 0 ? this->line_ : case_expr_node_ref.line, 
                                     case_expr_node_ref.column == 0 ? this->column_ : case_expr_node_ref.column);
            }
            
            bool values_equal = false;
            try {
                values_equal = (switch_value->get<int>() == case_expr_value->get<int>());
            } catch (const std::runtime_error& e) {
                 throw ::Interpreter::Exception(std::string("Error during case value comparison: ") + e.what(), 
                                     case_expr_node_ref.filename.empty() ? this->filename_ : case_expr_node_ref.filename, 
                                     case_expr_node_ref.line == 0 ? this->line_ : case_expr_node_ref.line, 
                                     case_expr_node_ref.column == 0 ? this->column_ : case_expr_node_ref.column);
            }

            if (values_equal) {
                matched_block_found = true;
            }
        }

        if (matched_block_found) {
            try {
                for (const auto& stmt_node : case_block.statements) {
                    stmt_node->interpret(interpreter); // CHANGED from Accept to interpret
                }
            } catch (const ::Interpreter::BreakException&) { 
                break_executed = true;
            } catch (const ::Interpreter::Exception&) { 
                throw; 
            } catch (const std::runtime_error& e) { 
                throw ::Interpreter::Exception(e.what(), this->filename_, this->line_, this->column_);
            }
        }
    }

    if (!break_executed && this->defaultBlock) {
        // If defaultBlock exists and we haven't broken out of the switch yet by a 'break', execute it.
        // This handles fall-through cases as well.
        try {
            for (const auto& stmt_node : this->defaultBlock.value().statements) {
                stmt_node->interpret(interpreter); // CHANGED from Accept to interpret
            }
        } catch (const ::Interpreter::BreakException&) {
            // A break in the default block will end the switch.
            // break_executed = true; // Not strictly necessary as the switch statement is ending.
        } catch (const ::Interpreter::Exception&) {
            throw;
        } catch (const std::runtime_error& e) {
            throw ::Interpreter::Exception(e.what(), this->filename_, this->line_, this->column_);
        }
    }
}

} // namespace Interpreter::Nodes::Statement
