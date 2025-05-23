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
            
            // Add debug information
            std::cout << "DEBUG: ReturnStatementNode - Returning value of type: " 
                      << Symbols::Variables::TypeToString(retVal->getType()) << std::endl;
            
            // Handle binary operations returning as objects instead of booleans
            if (retVal->getType() == Symbols::Variables::Type::OBJECT) {
                std::cout << "DEBUG: ReturnStatementNode - Examining OBJECT return value" << std::endl;
                
                try {
                    // Look for binary operation patterns
                    const Symbols::ObjectMap& objMap = retVal->get<Symbols::ObjectMap>();
                    
                    // If this is a comparison result that should be a boolean
                    if (objMap.find("left") != objMap.end() && objMap.find("right") != objMap.end() && 
                        objMap.find("operator") != objMap.end()) {
                        
                        std::cout << "DEBUG: ReturnStatementNode - Found binary operation structure" << std::endl;
                        
                        auto leftIt = objMap.find("left");
                        auto rightIt = objMap.find("right");
                        auto opIt = objMap.find("operator");
                        
                        if (leftIt->second->getType() == Symbols::Variables::Type::INTEGER && 
                            rightIt->second->getType() == Symbols::Variables::Type::INTEGER && 
                            opIt->second->getType() == Symbols::Variables::Type::STRING) {
                            
                            int left = leftIt->second->get<int>();
                            int right = rightIt->second->get<int>();
                            std::string op = opIt->second->get<std::string>();
                            
                            std::cout << "DEBUG: ReturnStatementNode - Evaluating " << left << " " << op << " " << right << std::endl;
                            
                            bool result = false;
                            if (op == ">") result = (left > right);
                            else if (op == "<") result = (left < right);
                            else if (op == ">=") result = (left >= right);
                            else if (op == "<=") result = (left <= right);
                            else if (op == "==") result = (left == right);
                            else if (op == "!=") result = (left != right);
                            
                            std::cout << "DEBUG: ReturnStatementNode - Computed boolean result: " << (result ? "true" : "false") << std::endl;
                            retVal = Symbols::ValuePtr(result);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cout << "DEBUG: ReturnStatementNode - Error examining object: " << e.what() << std::endl;
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
