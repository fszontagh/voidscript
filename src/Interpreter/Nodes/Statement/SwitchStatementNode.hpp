#ifndef SWITCH_STATEMENT_NODE_HPP
#define SWITCH_STATEMENT_NODE_HPP

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <optional> // For std::optional

#include "../../StatementNode.hpp" // Base class
#include "../../ExpressionNode.hpp" // For ExpressionNode
#include "../../../Interpreter/Interpreter.hpp" // For Interpreter class (forward declaration might be needed if Interpreter.hpp includes this)

// Forward declare Interpreter to avoid circular dependency issues if not fully including Interpreter.hpp
// class Interpreter::Interpreter; // This is not standard, better to include Interpreter.hpp

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

    void Accept(::Interpreter::Interpreter& interpreter) const;

    // interpret() is pure virtual in StatementNode.
    void interpret(::Interpreter::Interpreter& interpreter) const override;

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
