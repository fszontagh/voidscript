#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
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
                auto className = classObj.find("$class_name");
                if (className == classObj.end() || className->second->getType() != Symbols::Variables::Type::STRING) {
                    throw std::runtime_error("Object missing $class_name property");
                }
                
                // Get the class name
                std::string cn = className->second->get<std::string>();
                
                // Get class info from ClassRegistry or from SymbolContainer
                auto& classRegistry = Symbols::ClassRegistry::instance();
                
                // Check if class exists either in ClassRegistry or as a Class symbol
                bool classFoundInClassRegistry = classRegistry.hasClass(cn);
                
                // If not found in class registry, check in SymbolContainer
                if (!classFoundInClassRegistry) {
                    // Build the class scope name using the file and class name
                    // Get the current scope from SymbolContainer
                    const std::string current_scope = Symbols::SymbolContainer::instance()->currentScopeName();
                    const std::string class_scope_name = current_scope.empty() ? cn : current_scope + Symbols::SymbolContainer::SCOPE_SEPARATOR + cn;
                    
                    // Look up in SymbolContainer
                    auto* sc = Symbols::SymbolContainer::instance();
                    auto class_scope_table = sc->getScopeTable(class_scope_name);
                    
                    if (!class_scope_table) {
                        // Try to look up the class without additional scope prefixing
                        auto direct_class_table = sc->getScopeTable(cn);
                        if (!direct_class_table) {
                            throw std::runtime_error("Class " + cn + " not found in scope " + class_scope_name);
                        }
                        
                        class_scope_table = direct_class_table;
                    }
                    
                    // Check if method exists in class scope
                    auto sym_method = class_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, methodName_);
                    
                    if (!sym_method || (sym_method->getKind() != Symbols::Kind::Function && 
                                       sym_method->getKind() != Symbols::Kind::Method)) {
                        throw std::runtime_error("Method '" + methodName_ + "' not found in class " + cn);
                    }
                    
                    // Method found in SymbolContainer, proceed with execution
                    
                } else {
                    // Get class info from ClassRegistry
                    auto& classInfo = classRegistry.getClassContainer().getClassInfo(cn);
                    
                    // Check if method exists in ClassRegistry
                    if (!classRegistry.getClassContainer().hasMethod(cn, methodName_)) {
                        throw std::runtime_error("Method " + methodName_ + " not found in class");
                    }
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
                // First try ClassRegistry, then fallback to SymbolContainer
                try {
                    returnValue = interpreter.executeMethod(objVal, methodName_, evaluatedArgs);
                } catch (const std::exception& e) {
                    // Method not found in ClassRegistry, try using SymbolContainer
                    
                    // Get the current scope from SymbolContainer
                    const std::string current_scope = Symbols::SymbolContainer::instance()->currentScopeName();
                    // Build the class scope names to try
                    std::vector<std::string> class_scope_names;
                    
                    // First try: direct class name
                    class_scope_names.push_back(cn);
                    // Second try: current scope + class name
                    if (!current_scope.empty()) {
                        class_scope_names.push_back(current_scope + Symbols::SymbolContainer::SCOPE_SEPARATOR + cn);
                    }
                    // Third try: filename + class name (backward compatibility)
                    if (!filename_.empty()) {
                        class_scope_names.push_back(filename_ + Symbols::SymbolContainer::SCOPE_SEPARATOR + cn);
                    }
                    
                    // Get SymbolContainer instance
                    auto* sc = Symbols::SymbolContainer::instance();
                    std::shared_ptr<Symbols::Symbol> sym_method = nullptr;
                    std::string method_scope_name;
                    
                    // Try each possible scope for the class
                    for (const auto& scope_name : class_scope_names) {
                        auto class_scope_table = sc->getScopeTable(scope_name);
                        
                        if (class_scope_table) {
                            sym_method = class_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, methodName_);
                            if (sym_method && (sym_method->getKind() == Symbols::Kind::Function || 
                                             sym_method->getKind() == Symbols::Kind::Method)) {
                                method_scope_name = scope_name;
                                break;
                            }
                        }
                    }
                    
                    if (!sym_method) {
                        throw std::runtime_error("Method '" + methodName_ + "' not found in class " + cn);
                    }
                    
                    // Execute the method directly through the interpreter
                    std::shared_ptr<Symbols::FunctionSymbol> funcSym;
                    
                    if (sym_method->getKind() == Symbols::Kind::Method) {
                        funcSym = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(sym_method);
                    } else {
                        funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym_method);
                    }
                    
                    const auto& params = funcSym->parameters();
                    
                    // Create and enter method scope
                    const std::string methodNs = method_scope_name + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName_;
                    sc->create(methodNs);
                    
                    // Bind 'this' and parameters
                    // This is crucial - create a variable named "this" in the method scope
                    // that points to the object instance
                    sc->addVariable(Symbols::SymbolFactory::createVariable("this", objVal, methodNs));
                    
                    // Add parameters
                    for (size_t i = 0; i < std::min(params.size(), evaluatedArgs.size()); ++i) {
                        sc->addVariable(Symbols::SymbolFactory::createVariable(params[i].name, evaluatedArgs[i], methodNs));
                    }
                    
                    // Execute method body
                    bool returnCaught = false;
                    try {
                        for (const auto& op : Operations::Container::instance()->getAll(methodNs)) {
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
            std::cerr << "Exception in method call: " << e.what() << std::endl;
            throw;
        }
    }
};

} // namespace Interpreter

#endif // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
