#include "Interpreter/Interpreter.hpp"

#include <iostream>

#include "Interpreter/ReturnException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/EnumSymbol.hpp" // Added for EnumSymbol
#include "Nodes/Statement/EnumDeclarationNode.hpp" // Added for EnumDeclarationNode
#include "Nodes/Statement/SwitchStatementNode.hpp" // Added for SwitchStatementNode
#include "Nodes/Statement/BreakNode.hpp"         // Added for BreakNode
#include "Interpreter/BreakException.hpp"        // Added for BreakException
#include "Interpreter/ExpressionNode.hpp"        // For ExpressionNode::evaluate


namespace Interpreter {

void Interpreter::setThisObject(const Symbols::ValuePtr & obj) {
    thisObject_ = obj;
}

void Interpreter::clearThisObject() {
    thisObject_ = Symbols::ValuePtr();
}

const Symbols::ValuePtr & Interpreter::getThisObject() const {
    return thisObject_;
}

void Interpreter::run() {
    // Determine namespace to execute
    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
    for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
        runOperation(*operation);
    }
}

void Interpreter::runOperation(const Operations::Operation & op) {
    if (debug_) {
        std::cerr << "[Debug][Interpreter] Operation: " << op.toString() << "\n";
    }

    if (!op.statement && op.type != Operations::Type::Error) {
        throw Exception("Invalid operation: missing statement", "-", 0, 0);
    }

    try {
        switch (op.type) {
            // Basic statements
            case Operations::Type::Declaration:
            case Operations::Type::Assignment:
            case Operations::Type::Expression:
                op.statement->interpret(*this);
                break;

            // Function-related operations
            case Operations::Type::FuncDeclaration:
            case Operations::Type::MethodDeclaration:
            case Operations::Type::FunctionCall:
            case Operations::Type::MethodCall:
            case Operations::Type::Return:
                op.statement->interpret(*this);
                break;

            // Control flow
            case Operations::Type::Conditional:
            case Operations::Type::Loop:
            case Operations::Type::While:
                op.statement->interpret(*this);
                break;

            // Flow control statements
            case Operations::Type::Break:
            case Operations::Type::Continue:
                throw Exception("Break/Continue not implemented", "-", 0, 0);

            // Module system
            case Operations::Type::Import:
                throw Exception("Import not implemented", "-", 0, 0);

            // Special cases
            case Operations::Type::Block:
                op.statement->interpret(*this);
                break;

            case Operations::Type::Error:
                throw Exception("Error operation encountered", "-", 0, 0);

            default:
                throw Exception("Unknown operation type", "-", 0, 0);
        }
    } catch (const Exception &) {
        throw;
    } catch (const std::exception & e) {
        throw Exception(e.what(), "-", 0, 0);
    }
}

unsigned long long Interpreter::get_unique_call_id() {
    return next_call_id_++;
}

// Visitor method implementations
void Interpreter::Visit(const Nodes::Statement::EnumDeclarationNode& node) {
    std::string current_scope_name = Symbols::SymbolContainer::instance()->currentScopeName();
    
    // Context for the EnumSymbol (e.g., for error messages if needed later)
    // Using node.filename_, node.line_, node.column_ which are base StatementNode members
    std::string context_str = node.filename_ + ":" + std::to_string(node.line_) + ":" + std::to_string(node.column_);

    try {
        // Create the EnumSymbol. The EnumSymbol constructor handles enumerator logic.
        // The EnumSymbol itself is not directly "evaluated" to a ValuePtr in the same way an expression is.
        // It's a definition that's stored.
        // BaseSymbol expects a name, a ValuePtr (can be null for non-value symbols like types), context, and kind.
        // The EnumSymbol constructor should correctly call its base Symbol constructor.
        auto enumSymbol = std::make_shared<Symbols::EnumSymbol>(
            node.enumName,
            node.enumerators,
            context_str // This context is for the EnumSymbol itself, not its internal values.
                        // The EnumSymbol constructor uses its own name and this context for the base Symbol.
        );

        // Add the EnumSymbol to the symbol table in the current scope.
        // The SymbolContainer::add method takes a std::shared_ptr<Symbol>.
        Symbols::SymbolContainer::instance()->add(enumSymbol);

        if (debug_) {
            std::cerr << "[Debug][Interpreter] Declared enum: " << node.enumName 
                      << " in scope: " << current_scope_name 
                      << " with context: " << context_str << std::endl;
        }

    } catch (const std::runtime_error& e) {
        // Catch errors from EnumSymbol constructor (e.g., duplicate enumerator names)
        // or from SymbolContainer (e.g., duplicate symbol name in scope)
        throw Interpreter::Exception(e.what(), node.filename_, node.line_, node.column_);
    }
}

// Implement Visit for SwitchStatementNode and BreakNode in the next subtask.

void Interpreter::Visit(const Nodes::Statement::BreakNode& node) {
    if (debug_) {
        std::cerr << "[Debug][Interpreter] Executing BreakNode at "
                  << node.filename_ << ":" << node.line_ << ":" << node.column_ << std::endl;
    }
    throw BreakException();
}

void Interpreter::Visit(const Nodes::Statement::SwitchStatementNode& node) {
    if (debug_) {
        std::cerr << "[Debug][Interpreter] Executing SwitchStatementNode at "
                  << node.filename_ << ":" << node.line_ << ":" << node.column_ << std::endl;
    }

    Symbols::ValuePtr switch_value = node.switchExpression->evaluate(*this, node.filename_, node.line_, node.column_);
    
    // Validate switch expression type
    if (!switch_value || switch_value->isNULL() || switch_value->getType() != Symbols::Variables::Type::INTEGER) {
        // Use location from the switch expression node if possible, otherwise the switch statement node
        const auto& expr_node = *node.switchExpression; // Assuming switchExpression is not null
        throw Interpreter::Exception("Switch expression must evaluate to a non-null integer type.", 
                                     expr_node.filename.empty() ? node.filename_ : expr_node.filename, 
                                     expr_node.line == 0 ? node.line_ : expr_node.line, 
                                     expr_node.column == 0 ? node.column_ : expr_node.column);
    }

    bool matched_block_found = false;
    bool break_executed = false;

    for (const auto& case_block : node.caseBlocks) {
        if (break_executed) {
            break; 
        }

        if (!matched_block_found) {
            // Use location from the case expression node if possible
            const auto& case_expr_node_ref = *case_block.expression;
            Symbols::ValuePtr case_expr_value = case_block.expression->evaluate(*this, 
                                                                              case_expr_node_ref.filename.empty() ? node.filename_ : case_expr_node_ref.filename, 
                                                                              case_expr_node_ref.line == 0 ? node.line_ : case_expr_node_ref.line, 
                                                                              case_expr_node_ref.column == 0 ? node.column_ : case_expr_node_ref.column);
            
            // Validate case expression type
            if (!case_expr_value || case_expr_value->isNULL() || case_expr_value->getType() != Symbols::Variables::Type::INTEGER) {
                 throw Interpreter::Exception("Case expression must evaluate to a non-null integer type.", 
                                     case_expr_node_ref.filename.empty() ? node.filename_ : case_expr_node_ref.filename, 
                                     case_expr_node_ref.line == 0 ? node.line_ : case_expr_node_ref.line, 
                                     case_expr_node_ref.column == 0 ? node.column_ : case_expr_node_ref.column);
            }
            
            // Types are now guaranteed to be INTEGER and non-null.
            bool values_equal = false;
            try {
                // Comparison is now only between integers.
                values_equal = (switch_value->get<int>() == case_expr_value->get<int>());
            } catch (const std::runtime_error& e) { // Should be less likely now with type checks
                 throw Interpreter::Exception(std::string("Error during case value comparison: ") + e.what(), 
                                     case_expr_node_ref.filename.empty() ? node.filename_ : case_expr_node_ref.filename, 
                                     case_expr_node_ref.line == 0 ? node.line_ : case_expr_node_ref.line, 
                                     case_expr_node_ref.column == 0 ? node.column_ : case_expr_node_ref.column);
            }

            if (values_equal) {
                matched_block_found = true;
            }
        }

        if (matched_block_found) {
            try {
                for (const auto& stmt_node : case_block.statements) {
                    // Provide location of the statement being executed if possible
                    // For now, using switch statement's location as a fallback for exceptions within statements
                    stmt_node->Accept(*this); 
                }
            } catch (const BreakException&) {
                break_executed = true;
            } catch (const Interpreter::Exception& e) { // Catch interpreter exceptions to rethrow
                throw; 
            } catch (const std::runtime_error& e) { // Catch other runtime errors
                throw Interpreter::Exception(e.what(), node.filename_, node.line_, node.column_);
            }
        }
    }

    if (!break_executed && node.defaultBlock) {
        // Default block executes if no case matched, OR if a matched case fell through into default.
        // The `matched_block_found` flag correctly handles the fall-through scenario.
        // If no case matched (!matched_block_found), default executes.
        // If a case matched and fell through (matched_block_found is true), default executes.
        if (matched_block_found || !node.caseBlocks.empty() || node.caseBlocks.empty()) { // Simplified: if default exists and no break, execute.
                                                                                        // More precise: execute if (!matched_block_found || (matched_block_found && !break_executed_from_that_specific_block_just_before_default))
                                                                                        // The current `matched_block_found` correctly handles fall-through into default.
                                                                                        // If no cases matched, `matched_block_found` is false, default executes.
                                                                                        // If cases exist and none matched, default executes.
                                                                                        // If cases exist and one matched and fell through, `matched_block_found` is true, default executes.
            if (!matched_block_found || matched_block_found) { // This condition simplifies to: always execute default if not broken out and default exists.
                                                               // More accurate: if (!any_case_matched_before_default || we_are_falling_through_into_default)
                                                               // The prompt's logic: "If node.defaultBlock exists AND case_matched is false (or if fall-through from a non-breaking case leads here)"
                                                               // My current logic: if break_executed is false, and defaultBlock exists, then if (matched_block_found || no cases matched)
                                                               // This can be simplified: if default exists and we haven't broken out, it executes if EITHER no case has matched yet, OR we are in fall-through mode.
                                                               // The `matched_block_found` flag handles this:
                                                               //  - If no case matched, `matched_block_found` is false. Default should run.
                                                               //  - If a case matched and didn't break, `matched_block_found` is true. Default should run (fall-through).
                                                               // So, the condition is simply: if defaultBlock exists (and we haven't broken out)
                 if (debug_) {
                    std::cerr << "[Debug][Interpreter] Executing default block." << std::endl;
                }
                try {
                    for (const auto& stmt_node : node.defaultBlock.value().statements) {
                         stmt_node->Accept(*this); //or stmt_node->interpret(*this);
                    }
                } catch (const BreakException&) {
                    // break_executed = true; // Switch ends after this anyway
                }
            }
        }
    }
}

}  // namespace Interpreter
