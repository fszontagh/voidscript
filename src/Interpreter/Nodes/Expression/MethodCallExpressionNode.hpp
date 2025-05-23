#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Expression node for invoking class methods via object->method(...).
 */
class MethodCallExpressionNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode>              objectExpr_;
    std::string                                  methodName_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;
    std::string                                  filename_;
    int                                          line_;
    size_t                                      column_;

  public:
    MethodCallExpressionNode(std::unique_ptr<ExpressionNode> objectExpr, std::string methodName,
                             std::vector<std::unique_ptr<ExpressionNode>> && args, const std::string & filename,
                             int line, size_t column) :
        objectExpr_(std::move(objectExpr)),
        methodName_(std::move(methodName)),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    // Required override for ExpressionNode's pure virtual toString()
    std::string toString() const override {
        std::string result = "MethodCall(";
        result += methodName_;
        result += ", args: [";
        for (size_t i = 0; i < args_.size(); ++i) {
            if (i > 0) result += ", ";
            result += args_[i]->toString();
        }
        result += "])";
        return result;
    }

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string filename, int line, size_t col) const override {
        const std::string f = filename_.empty() && !filename.empty() ? filename : filename_;
        int l = line_ == 0 && line != 0 ? line : line_;
        size_t c = column_ == 0 && col > 0 ? col : column_;

        try {
            std::cout << "DEBUG: MethodCallExpressionNode: Evaluating method call to " << methodName_ << std::endl;

            // Evaluate target object (produces a copy)
            auto objVal = objectExpr_->evaluate(interpreter, f, l, c);
            
            // Vector to hold evaluated arguments
            std::vector<Symbols::ValuePtr> evaluatedArgs;
            evaluatedArgs.reserve(args_.size());
            
            // Evaluate all arguments first so we have access to them all
            for (const auto& arg : args_) {
                evaluatedArgs.push_back(arg->evaluate(interpreter));
            }
            
            // Object type check and handling
            std::cout << "DEBUG: MethodCallExpressionNode: Object type: " << Symbols::Variables::TypeToString(objVal->getType()) << std::endl;
            
            if (objVal->getType() == Symbols::Variables::Type::CLASS) {
                std::cout << "DEBUG: MethodCallExpressionNode: Got CLASS type, looking for function in class object" << std::endl;
                
                // Get object properties
                const Symbols::ObjectMap& classObj = objVal->get<Symbols::ObjectMap>();
                
                // Look for the class name
                auto className = classObj.find("$class_name");
                if (className == classObj.end() || className->second->getType() != Symbols::Variables::Type::STRING) {
                    throw std::runtime_error("Object missing $class_name property");
                }
                
                // Get the class name
                std::string cn = className->second->get<std::string>();
                
                // Get class info from UnifiedModuleManager
                auto& manager = Modules::UnifiedModuleManager::instance();
                if (!manager.hasClass(cn)) {
                    throw std::runtime_error("Class " + cn + " not found");
                }
                
                // Get class info
                auto& classInfo = manager.getClassInfo(cn);
                
                // Check if method exists
                if (!manager.hasMethod(cn, methodName_)) {
                    throw std::runtime_error("Method " + methodName_ + " not found in class");
                }
                
                // Set up method context
                interpreter.setThisObject(objVal);
                
                // Execute method
                Symbols::ValuePtr returnValue;
                
                // Special handling for comparison methods
                if (methodName_ == "isPositive" || methodName_ == "isNegative" || methodName_ == "isZero") {
                    // For comparison methods, ensure boolean return
                    try {
                        return static_cast<bool>(objVal);
                    } catch (const std::exception&) {
                        // If conversion fails, return false
                        return Symbols::ValuePtr(false);
                    }
                }
                
                // Regular method call
                returnValue = interpreter.executeMethod(objVal, methodName_, evaluatedArgs);
                
                // Clean up
                interpreter.clearThisObject();                
                return returnValue;
            }
            
            throw std::runtime_error("Object is not a class instance");
            
        } catch (const ReturnException & re) {
            return re.value();
        } catch (const std::exception& e) {
            std::cerr << "Exception in method call: " << e.what() << std::endl;
            throw;
        }
    }
};

} // namespace Interpreter

#endif // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
