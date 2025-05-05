#ifndef INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP
#define INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP

#include <vector>
#include <memory>
#include <string>
#include "Interpreter/StatementNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Interpreter/Nodes/Statement/DeclareVariableStatementNode.hpp"
#include <iostream>

namespace Interpreter {

/**
 * @brief Statement node representing a C-style for loop: for(init; cond; incr) { body }
 */
class CStyleForStatementNode : public StatementNode {
private:
    std::unique_ptr<StatementNode> initStmt_;
    std::unique_ptr<ExpressionNode> condExpr_;
    std::unique_ptr<StatementNode> incrStmt_;
    std::vector<std::unique_ptr<StatementNode>> body_;

public:
    CStyleForStatementNode(std::unique_ptr<StatementNode> initStmt,
                           std::unique_ptr<ExpressionNode> condExpr,
                           std::unique_ptr<StatementNode> incrStmt,
                           std::vector<std::unique_ptr<StatementNode>> body,
                           const std::string & file_name,
                           int line,
                           size_t column)
      : StatementNode(file_name, line, column),
        initStmt_(std::move(initStmt)),
        condExpr_(std::move(condExpr)),
        incrStmt_(std::move(incrStmt)),
        body_(std::move(body)) {}

    void interpret(Interpreter & interpreter) const override {
        std::string loop_scope_name; // Keep track of scope to exit
        bool entered_scope = false;
        try {
            using namespace Symbols;
            auto * symContainer = SymbolContainer::instance();
            // Get loop scope name from the init declaration statement
            if (auto* declStmt = dynamic_cast<const DeclareVariableStatementNode*>(initStmt_.get())) {
                 loop_scope_name = declStmt->getNamespace(); 
                 if (!loop_scope_name.empty()) {
                    // Enter the scope created by the parser
                    symContainer->enter(loop_scope_name); 
                    entered_scope = true;
                 } else {
                    //  std::cerr << "[ERROR][CStyleFor] Init declaration node missing namespace!" << std::endl;
                     throw Exception("C-style loop init declaration node missing namespace.", filename_, line_, column_);
                 }
            } else {
                 // std::cerr << "[ERROR][CStyleFor] Init statement is not a DeclareVariableStatementNode!" << std::endl;
                 throw Exception("C-style for loop internal error: init statement is not a declaration.", filename_, line_, column_);
            }
            
            initStmt_->interpret(interpreter);
            
            // Loop condition and body (executed within loop scope)
            while (true) {
                Value condVal = condExpr_->evaluate(interpreter);
                if (condVal.getType() != Variables::Type::BOOLEAN) {
                     std::cerr << "[ERROR][CStyleFor] Condition not boolean!" << std::endl;
                    if (entered_scope) symContainer->enterPreviousScope(); // Exit scope before throwing
                    throw Exception("For loop condition not boolean", filename_, line_, column_);
                }
                bool shouldContinue = condVal.get<bool>();
                if (!shouldContinue) break;
                
                for (const auto & stmt : body_) {
                     if (stmt) {
                        stmt->interpret(interpreter);
                    }
                }
                 
                if (incrStmt_) {
                    incrStmt_->interpret(interpreter);
                }
            }
            
            // --- Scope Cleanup ---
            if (entered_scope) {
                 symContainer->enterPreviousScope(); // Exit the loop scope
            }
             
        } catch (const Exception &e) {
             //std::cerr << "[ERROR][CStyleFor] Exception caught: " << e.what() << std::endl;
            if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope(); // Ensure exit on exception
            }
            throw;
        } catch (const std::exception & e) {
             //std::cerr << "[ERROR][CStyleFor] std::exception caught: " << e.what() << std::endl;
             if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope(); // Ensure exit on exception
            }
            throw Exception(e.what(), filename_, line_, column_);
        }
        if (entered_scope) {
        
            Symbols::SymbolContainer::instance()->enterPreviousScope();
        }
    }

    std::string toString() const override {
        return "CStyleForStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
};

} // namespace Interpreter

#endif // INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP