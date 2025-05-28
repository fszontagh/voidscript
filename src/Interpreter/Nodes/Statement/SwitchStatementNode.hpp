#ifndef SWITCH_STATEMENT_NODE_HPP
#define SWITCH_STATEMENT_NODE_HPP

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <optional> // For std::optional

#include "../../StatementNode.hpp" // Base class
#include "../../ExpressionNode.hpp" // For ExpressionNode
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class, Exception, ValuePtr, Variables::Type
#include "../../../Interpreter/BreakException.hpp" // For BreakException

// Forward declare Interpreter is no longer needed due to direct include of Interpreter.hpp

namespace Interpreter::Nodes::Statement {

class SwitchStatementNode : public ::Interpreter::StatementNode {
public:
    // Nested structure for CaseBlock
    struct CaseBlock {
        std::unique_ptr<::Interpreter::ExpressionNode> expression;
        std::vector<std::unique_ptr<::Interpreter::StatementNode>> statements;

        CaseBlock(
            std::unique_ptr<::Interpreter::ExpressionNode> expr,
            std::vector<std::unique_ptr<::Interpreter::StatementNode>> stmts
        ) : expression(std::move(expr)), statements(std::move(stmts)) {}
    };

    // Nested structure for DefaultBlock
    struct DefaultBlock {
        std::vector<std::unique_ptr<::Interpreter::StatementNode>> statements;

        explicit DefaultBlock(
            std::vector<std::unique_ptr<::Interpreter::StatementNode>> stmts
        ) : statements(std::move(stmts)) {}
    };

    std::unique_ptr<::Interpreter::ExpressionNode> switchExpression;
    std::vector<CaseBlock> caseBlocks;
    std::optional<DefaultBlock> defaultBlock;

    SwitchStatementNode(
        const std::string& file_name,
        int file_line,
        size_t line_column,
        std::unique_ptr<::Interpreter::ExpressionNode> switch_expr,
        std::vector<CaseBlock> cases,
        std::optional<DefaultBlock> default_case
    ) : ::Interpreter::StatementNode(file_name, file_line, line_column),
        switchExpression(std::move(switch_expr)),
        caseBlocks(std::move(cases)),
        defaultBlock(std::move(default_case)) {}

    void Accept(::Interpreter::Interpreter& interpreter) const {
        this->interpret(interpreter);
    }

    // interpret() is pure virtual in StatementNode.
    void interpret(::Interpreter::Interpreter& interpreter) const override {
        // Copied logic from SwitchStatementNode.cpp's interpret method:
        ::Symbols::ValuePtr switch_value = this->switchExpression->evaluate( // Added ::Symbols
            interpreter, this->filename_, this->line_, this->column_
        );
        
        if (!switch_value || switch_value->isNULL() || switch_value->getType() != ::Symbols::Variables::Type::INTEGER) { // Qualified Type
            const auto& expr_node = *this->switchExpression;
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
                ::Symbols::ValuePtr case_expr_value = case_block.expression->evaluate( // Added ::Symbols
                    interpreter, 
                    case_expr_node_ref.filename.empty() ? this->filename_ : case_expr_node_ref.filename, 
                    case_expr_node_ref.line == 0 ? this->line_ : case_expr_node_ref.line, 
                    case_expr_node_ref.column == 0 ? this->column_ : case_expr_node_ref.column
                );
                
                if (!case_expr_value || case_expr_value->isNULL() || case_expr_value->getType() != ::Symbols::Variables::Type::INTEGER) { // Qualified Type
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
                        stmt_node->interpret(interpreter); 
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
            try {
                for (const auto& stmt_node : this->defaultBlock.value().statements) {
                    stmt_node->interpret(interpreter); 
                }
            } catch (const ::Interpreter::BreakException&) {
                // Break in default block ends switch.
            } catch (const ::Interpreter::Exception&) {
                throw;
            } catch (const std::runtime_error& e) {
                throw ::Interpreter::Exception(e.what(), this->filename_, this->line_, this->column_);
            }
        }
    }

    // It's good practice to have a toString for debugging
    std::string toString() const override {
        std::string str = "SwitchStatementNode(\n";
        str += "  SwitchExpression: " + (switchExpression ? switchExpression->toString() : "nullptr") + ",\n";
        str += "  CaseBlocks: [\n";
        for (const auto& case_block : caseBlocks) {
            str += "    CaseBlock(\n";
            str += "      Expression: " + (case_block.expression ? case_block.expression->toString() : "nullptr") + ",\n";
            str += "      Statements: [\n";
            for (const auto& stmt : case_block.statements) {
                str += "        " + (stmt ? stmt->toString() : "nullptr") + ",\n";
            }
            str += "      ]\n";
            str += "    ),\n";
        }
        str += "  ],\n";
        if (defaultBlock.has_value()) {
            str += "  DefaultBlock: (\n";
            str += "    Statements: [\n";
            for (const auto& stmt : defaultBlock.value().statements) {
                str += "      " + (stmt ? stmt->toString() : "nullptr") + ",\n";
            }
            str += "    ]\n";
            str += "  )\n";
        } else {
            str += "  DefaultBlock: None\n";
        }
        str += ")";
        return str;
    }
};

} // namespace Interpreter::Nodes::Statement

#endif // SWITCH_STATEMENT_NODE_HPP
