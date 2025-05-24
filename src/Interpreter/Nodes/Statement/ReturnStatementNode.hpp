// ReturnStatementNode.hpp
#ifndef INTERPRETER_RETURN_STATEMENT_NODE_HPP
#define INTERPRETER_RETURN_STATEMENT_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node representing a return statement inside a function.
 */
class ReturnStatementNode : public StatementNode {
    std::unique_ptr<ExpressionNode> expr_;
  public:
    explicit ReturnStatementNode(std::unique_ptr<ExpressionNode> expr, const std::string & file_name, int line,
                                 size_t column) :
        StatementNode(file_name, line, column),
        expr_(std::move(expr)) {}

    void interpret(Interpreter & interpreter) const override {
        Symbols::ValuePtr retVal;
        if (expr_) {
            retVal = expr_->evaluate(interpreter);
            
            // Handle binary operations returning as objects instead of booleans
            if (retVal->getType() == Symbols::Variables::Type::OBJECT) {
                
                try {
                    // Look for binary operation patterns
                    const Symbols::ObjectMap& objMap = retVal->get<Symbols::ObjectMap>();
                    
                    // If this is a comparison result that should be a boolean
                    if (objMap.find("left") != objMap.end() && objMap.find("right") != objMap.end() && 
                        objMap.find("operator") != objMap.end()) {
                        
                        auto leftIt = objMap.find("left");
                        auto rightIt = objMap.find("right");
                        auto opIt = objMap.find("operator");
                        
                        if (leftIt->second->getType() == Symbols::Variables::Type::INTEGER && 
                            rightIt->second->getType() == Symbols::Variables::Type::INTEGER && 
                            opIt->second->getType() == Symbols::Variables::Type::STRING) {
                            
                            int left = leftIt->second->get<int>();
                            int right = rightIt->second->get<int>();
                            std::string op = opIt->second->get<std::string>();
                            
                            bool result = false;
                            if (op == ">") result = (left > right);
                            else if (op == "<") result = (left < right);
                            else if (op == ">=") result = (left >= right);
                            else if (op == "<=") result = (left <= right);
                            else if (op == "==") result = (left == right);
                            else if (op == "!=") result = (left != right);
                            
                            retVal = Symbols::ValuePtr(result);
                        }
                    }
                } catch (const std::exception& e) {
                    // Failed to examine object
                }
            }
        }
        throw ReturnException(retVal);
    }

    std::string toString() const override {
        return std::string("return") + (expr_ ? (" " + expr_->toString()) : std::string());
    }
};

}  // namespace Interpreter
#endif  // INTERPRETER_RETURN_STATEMENT_NODE_HPP
