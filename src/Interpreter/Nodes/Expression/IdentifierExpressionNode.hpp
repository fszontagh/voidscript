#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

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

        // Use a hierarchical find method starting from the current scope
        auto symbol = sc->findSymbol(name_);  // Now uses the implemented findSymbol

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
