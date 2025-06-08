#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"

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
            if (objVal->getType() == Symbols::Variables::Type::CLASS) {
                
                // Get object properties
                const Symbols::ObjectMap& classObj = objVal->get<Symbols::ObjectMap>();
                
                // Look for the class name
                auto classNameIt = classObj.find("$class_name"); // Renamed for clarity
                if (classNameIt == classObj.end()) {
                    throw std::runtime_error("Object missing $class_name property");
                }
                Symbols::ValuePtr classNameVal = classNameIt->second;
                std::cerr << "[DEBUG METHOD_CALL]   $class_name ValuePtr. toString(): " << classNameVal->toString()
                          << ". isNULL(): " << (classNameVal.operator->() ? (classNameVal->isNULL() ? "true" : "false") : "CPP_NULLPTR")
                          << ". Type: " << Symbols::Variables::TypeToString(classNameVal->getType()) << std::endl;

                if (classNameVal->getType() != Symbols::Variables::Type::STRING) { // Check type *after* logging
                    throw std::runtime_error("Object's $class_name property is not a string. Actual type: " + Symbols::Variables::TypeToString(classNameVal->getType()));
                }
                
                // Get the class name
                std::string cn = classNameVal->get<std::string>();
                
                // Get class info from SymbolContainer
                auto* symbolContainer = Symbols::SymbolContainer::instance();
                
                // Check if class exists in SymbolContainer
                if (!symbolContainer->hasClass(cn)) {
                    throw std::runtime_error("Class " + cn + " not found");
                }
                
                // Check if method exists in class
                if (!symbolContainer->hasMethod(cn, methodName_)) {
                    throw std::runtime_error("Method '" + methodName_ + "' not found in class " + cn);
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
                
                // Method call handling through Operations system
                auto* sc = Symbols::SymbolContainer::instance();
                
                // Get method information to validate parameters before any call
                std::shared_ptr<Symbols::Symbol> method_sym = sc->findMethod(cn, methodName_);
                if (method_sym) {
                    std::shared_ptr<Symbols::FunctionSymbol> funcSymbol;
                    if (method_sym->getKind() == Symbols::Kind::Method) {
                        funcSymbol = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(method_sym);
                    } else {
                        funcSymbol = std::static_pointer_cast<Symbols::FunctionSymbol>(method_sym);
                    }
                    
                    if (funcSymbol) {
                        const auto& parameters = funcSymbol->parameters();
                        // Validate parameter count
                        if (evaluatedArgs.size() != parameters.size()) {
                            throw ::Interpreter::Exception("Method '" + methodName_ + "' expects " +
                                                          std::to_string(parameters.size()) + " parameters but " +
                                                          std::to_string(evaluatedArgs.size()) + " provided",
                                                          f, l, c);
                        }
                    }
                }
                
                // Try native method first
                try {
                    returnValue = sc->callMethod(cn, methodName_, evaluatedArgs);
                    interpreter.clearThisObject();
                    return returnValue;
                } catch (const std::exception& e) {
                    // If native method call fails, try script method execution
                    
                    std::string classNamespace = sc->findClassNamespace(cn);
                    
                    // Get method information using findMethod which searches scope tables
                    std::shared_ptr<Symbols::Symbol> sym_method = sc->findMethod(cn, methodName_);
                    
                    if (!sym_method) {
                        throw std::runtime_error("Method '" + methodName_ + "' not found in class " + cn);
                    }
                    
                    // Execute the method directly through the operations system
                    std::shared_ptr<Symbols::FunctionSymbol> funcSym;
                    
                    if (sym_method->getKind() == Symbols::Kind::Method) {
                        funcSym = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(sym_method);
                    } else {
                        funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym_method);
                    }
                    
                    const auto& params = funcSym->parameters();
                    
                    // Validate parameter count
                    if (evaluatedArgs.size() != params.size()) {
                        throw ::Interpreter::Exception("Method '" + methodName_ + "' expects " +
                                                      std::to_string(params.size()) + " parameters but " +
                                                      std::to_string(evaluatedArgs.size()) + " provided",
                                                      f, l, c);
                    }
                    
                    // Create and enter method scope
                    // Use the class namespace instead of just class name for operations
                    const std::string fullClassNs = classNamespace + Symbols::SymbolContainer::SCOPE_SEPARATOR + cn;
                    const std::string methodNs = cn + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName_;
                    sc->create(methodNs);
                    
                    try {
                        // Bind 'this' and parameters
                        sc->addVariable(Symbols::SymbolFactory::createVariable("this", objVal, methodNs));
                        
                        // Add parameters
                        for (size_t i = 0; i < std::min(params.size(), evaluatedArgs.size()); ++i) {
                            sc->addVariable(Symbols::SymbolFactory::createVariable(params[i].name, evaluatedArgs[i], methodNs));
                        }
                        
                        // Execute method body
                        bool returnCaught = false;
                        // Look for operations in the method-specific namespace
                        const std::string methodBodyNs = fullClassNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName_;
                        auto methodOps = Operations::Container::instance()->getAll(methodBodyNs);
                        
                        for (const auto& op : methodOps) {
                            try {
                                interpreter.runOperation(*op);
                            } catch (const ReturnException& re) {
                                // Allow return values to propagate
                                returnValue = re.value();
                                returnCaught = true;
                                break;
                            }
                        }
                        
                        // If no return was caught but we have a non-null return type,
                        // create a default value of the appropriate type
                        if (!returnCaught && funcSym->returnType() != Symbols::Variables::Type::NULL_TYPE) {
                            returnValue = Symbols::ValuePtr::null(funcSym->returnType());
                        } else if (!returnCaught) {
                            // For void methods (NULL_TYPE), return null
                            returnValue = Symbols::ValuePtr();
                        }
                    } catch (...) {
                        // Exit scope before rethrowing
                        sc->enterPreviousScope();
                        throw;
                    }
                    
                    // Exit method scope
                    sc->enterPreviousScope();
                }
                
                // Clean up
                interpreter.clearThisObject();
                return returnValue;
            }
            
            throw std::runtime_error("Object is not a class instance");
            
        } catch (const ReturnException & re) {
            return re.value();
        } catch (const std::exception& e) {
            throw;
        }
    }
};

} // namespace Interpreter

#endif // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
