 #ifndef INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP
 #define INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP

 #include <vector>
 #include <memory>
 #include <string>
 #include "Interpreter/StatementNode.hpp"
 #include "Interpreter/Interpreter.hpp"
 #include "Interpreter/ExpressionNode.hpp"
 #include "Symbols/Value.hpp"

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
        try {
            using namespace Symbols;
            // Initialization
            initStmt_->interpret(interpreter);
            // Loop condition and body
            while (true) {
                Value condVal = condExpr_->evaluate(interpreter);
                if (condVal.getType() != Variables::Type::BOOLEAN) {
                    throw Exception("For loop condition not boolean", filename_, line_, column_);
                }
                if (!condVal.get<bool>()) break;
                for (const auto & stmt : body_) {
                    stmt->interpret(interpreter);
                }
                // Increment step
                incrStmt_->interpret(interpreter);
            }
        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return "CStyleForStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
};

} // namespace Interpreter

#endif // INTERPRETER_CSTYLEFORSTATEMENTNODE_HPP