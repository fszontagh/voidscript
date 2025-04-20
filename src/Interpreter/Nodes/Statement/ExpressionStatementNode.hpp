 #ifndef INTERPRETER_EXPRESSION_STATEMENT_NODE_HPP
 #define INTERPRETER_EXPRESSION_STATEMENT_NODE_HPP

 #include <memory>
 #include <string>
 #include "Interpreter/StatementNode.hpp"
 #include "Interpreter/ExpressionNode.hpp"

 namespace Interpreter {

 /**
  * @brief Statement node for evaluating an expression (e.g., method calls) as a statement.
  */
 class ExpressionStatementNode : public StatementNode {
     std::unique_ptr<ExpressionNode> expr_;
   public:
     ExpressionStatementNode(std::unique_ptr<ExpressionNode> expr,
                             const std::string &filename, int line, size_t column) :
         StatementNode(filename, line, column), expr_(std::move(expr)) {}

     void interpret(Interpreter &interpreter) const override {
         // Evaluate expression and discard result
         expr_->evaluate(interpreter);
     }

     std::string toString() const override {
         return std::string("ExpressionStatement");
     }
};

 } // namespace Interpreter

 #endif // INTERPRETER_EXPRESSION_STATEMENT_NODE_HPP