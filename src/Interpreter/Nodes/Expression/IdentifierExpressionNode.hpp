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
            // 'this' should typically be found directly in the current function call scope or a class scope it's nested within.
            auto thisSymbol = sc->getVariable("this"); 

            if (thisSymbol) {
                auto thisVal = thisSymbol->getValue();
                if (thisVal && (thisVal->getType() == Symbols::Variables::Type::CLASS || thisVal->getType() == Symbols::Variables::Type::OBJECT)) {
                    // thisVal is a CLASS or OBJECT type
                } else if (thisVal) {
                    // thisVal is some other type
                } else {
                    // thisVal is a nullptr
                }
                return thisVal;
            }
            // If thisSymbol is null after sc->getVariable("this")
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
