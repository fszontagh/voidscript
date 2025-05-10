#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class IdentifierExpressionNode : public ExpressionNode {
    std::string name_;

  public:
    explicit IdentifierExpressionNode(std::string name) : name_(std::move(name)) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        auto* sc = Symbols::SymbolContainer::instance();
        
        // Special handling for 'this' in method contexts
        if (name_ == "this") {
            // Check if we're in a method context by looking for double-colon in the current scope name
            std::string currentScope = sc->currentScopeName();
            if (currentScope.find("::") != std::string::npos) {
                // Try to find 'this' symbol using the hierarchical search
                auto symbol = sc->findSymbol("this");
                if (symbol) {
                    return symbol->getValue();
                }
                
                // If 'this' doesn't exist, log an error
                interpreter.debugLog("ERROR: 'this' variable not found in method scope: " + currentScope);
                
                // Extract class name from scope (format: file::className::methodName)
                std::string className;
                size_t lastColonPos = currentScope.rfind("::");
                if (lastColonPos != std::string::npos) {
                    size_t prevColonPos = currentScope.rfind("::", lastColonPos - 1);
                    if (prevColonPos != std::string::npos) {
                        className = currentScope.substr(prevColonPos + 2, lastColonPos - prevColonPos - 2);
                    }
                }
                
                if (!className.empty()) {
                    // Create a dummy 'this' object with __class__ metadata as a fallback
                    Symbols::Value::ObjectMap thisObj;
                    thisObj["__class__"] = Symbols::Value(className);
                    
                    interpreter.debugLog("Creating fallback 'this' object for class: " + className);
                    
                    // Create and add the 'this' variable to the current scope
                    auto thisSymbol = Symbols::SymbolFactory::createVariable(
                        "this", Symbols::Value(thisObj), currentScope, Symbols::Variables::Type::OBJECT);
                    sc->add(thisSymbol);
                    
                    return thisSymbol->getValue();
                }
            }
            
            // If we're not in a method context or couldn't create a fallback 'this',
            // throw a more descriptive error
            throw std::runtime_error("'this' can only be used within class methods (current scope: " + 
                                    sc->currentScopeName() + ")");
        }
        
        // Use a hierarchical find method starting from the current scope
        auto symbol = sc->findSymbol(name_);
        
        if (symbol) {
            return symbol->getValue();
        }
        
        // Handle built-in NULL literal
        if (name_ == "NULL" || name_ == "null") {
            return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
        }
        
        // If not found after hierarchical search, throw error
        throw std::runtime_error("Identifier '" + name_ + "' not found starting from scope: " + sc->currentScopeName());
    }

    std::string toString() const override { return name_; }
};

}  // namespace Interpreter

#endif  // IDENTIFIER_EXPRESSION_NODE_HPP
