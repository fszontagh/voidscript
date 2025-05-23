#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

#include <iostream> // Required for std::cerr
#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class IdentifierExpressionNode : public ExpressionNode {
    std::string name_;

  public:
    explicit IdentifierExpressionNode(std::string name) : name_(std::move(name)) {}

    Symbols::ValuePtr evaluate(Interpreter & /*interpreter*/, std::string /*filename*/, int /*line*/,
                               size_t /*column*/) const override {
        auto * sc = Symbols::SymbolContainer::instance();

        // Special handling for 'this' keyword
        if (name_ == "this") {
            std::cout << "DEBUG: IdentifierExpressionNode evaluating 'this' in scope: " << sc->currentScopeName() << std::endl;
            // 'this' should typically be found directly in the current function call scope or a class scope it's nested within.
            auto thisSymbol = sc->getVariable("this"); 

            if (thisSymbol) {
                auto thisVal = thisSymbol->getValue();
                if (thisVal && (thisVal->getType() == Symbols::Variables::Type::CLASS || thisVal->getType() == Symbols::Variables::Type::OBJECT) ) {
                    std::cout << "DEBUG: 'this' resolved to a CLASS or OBJECT. Type: " << Symbols::Variables::TypeToString(thisVal->getType()) << std::endl;
                    Symbols::ObjectMap& map = thisVal->get<Symbols::ObjectMap>();
                    std::cout << "DEBUG: 'this' object map keys: ";
                    for (const auto& pair : map) {
                        std::cout << pair.first << " (";
                        if(pair.second) {
                            std::cout << Symbols::Variables::TypeToString(pair.second->getType());
                        } else {
                            std::cout << "null_ptr";
                        }
                        std::cout << ") ";
                    }
                    std::cout << std::endl;
                } else if (thisVal) {
                    std::cout << "DEBUG: 'this' resolved but not to a CLASS or OBJECT. Type: " << Symbols::Variables::TypeToString(thisVal->getType()) << std::endl;
                } else {
                    std::cout << "DEBUG: 'this' resolved to a nullptr ValuePtr." << std::endl;
                }
                return thisVal;
            }
            // If thisSymbol is null after sc->getVariable("this")
            std::cout << "DEBUG: 'this' keyword not found in symbol table from scope: " << sc->currentScopeName() << std::endl;
            throw std::runtime_error("Keyword \'this\' not found in current context starting from scope: " + sc->currentScopeName());
        }

        // Try to get the variable first
        auto symbol = sc->getVariable(name_);
        
        // If not found as a variable, try as a constant
        if (!symbol) {
            symbol = sc->getConstant(name_);
        }
        
        // If not found as a variable or constant, try as a function
        if (!symbol) {
            symbol = sc->getFunction(name_);
        }
        
        // If still not found, try as a method
        if (!symbol) {
            symbol = sc->getMethod(name_);
        }

        if (symbol) {
            // Check if symbol is accessible (e.g., private members if applicable)
            // For now, assume accessible if found
            std::cerr << "[DEBUG IdentifierExpressionNode] Retrieving symbol '" << name_ << "'. State: " << symbol->getValue().toString() << std::endl;
            return symbol->getValue();
        }

        // Handle built-in NULL literal
        if (name_ == "NULL" || name_ == "null") {
            return Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
        }

        // If not found after hierarchical search, throw error
        // Report the specific scope where the search started for clarity
        throw std::runtime_error("Identifier '" + name_ + "' not found starting from scope: " + sc->currentScopeName());
    }

    std::string toString() const override { return name_; }
};

}  // namespace Interpreter

#endif  // IDENTIFIER_EXPRESSION_NODE_HPP
