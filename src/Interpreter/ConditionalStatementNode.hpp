 #ifndef INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP
 #define INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP

 #include <vector>
 #include <memory>
 #include <string>
#include "Interpreter/StatementNode.hpp"
// Include for unified runtime Exception
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ExpressionNode.hpp"

 namespace Interpreter {

 /**
  * @brief Statement node representing an if-else conditional block.
  */
 class ConditionalStatementNode : public StatementNode {
     std::unique_ptr<ExpressionNode> condition_;
     std::vector<std::unique_ptr<StatementNode>> thenBranch_;
     std::vector<std::unique_ptr<StatementNode>> elseBranch_;

   public:
     ConditionalStatementNode(
         std::unique_ptr<ExpressionNode> condition,
         std::vector<std::unique_ptr<StatementNode>> thenBranch,
         std::vector<std::unique_ptr<StatementNode>> elseBranch,
         const std::string & file_name,
         int line,
         size_t column
     ) : StatementNode(file_name, line, column),
         condition_(std::move(condition)),
         thenBranch_(std::move(thenBranch)),
         elseBranch_(std::move(elseBranch)) {}

     void interpret(class Interpreter & interpreter) const override {
         // Evaluate condition
         auto val = condition_->evaluate(interpreter);
         bool cond = false;
         if (val.getType() == Symbols::Variables::Type::BOOLEAN) {
             cond = val.get<bool>();
        } else {
            throw Exception("Condition did not evaluate to boolean", filename_, line_, column_);
        }
         // Execute appropriate branch
         const auto & branch = cond ? thenBranch_ : elseBranch_;
         for (const auto & stmt : branch) {
             stmt->interpret(interpreter);
         }
     }

     std::string toString() const override {
         return "ConditionalStatementNode at " + filename_ + ":" + std::to_string(line_);
     }
 };

 } // namespace Interpreter

 #endif // INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP