#ifndef INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP
#define INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node for assignments: variable or nested object property.
 *   e.g., $a = expr; or $obj->prop->sub = expr;
 */
class AssignmentStatementNode : public StatementNode {
  private:
    std::string                     targetName_;
    std::vector<std::string>        propertyPath_;
    std::unique_ptr<ExpressionNode> rhs_;
  public:
    AssignmentStatementNode(std::string targetName, std::vector<std::string> propertyPath,
                            std::unique_ptr<ExpressionNode> rhs, const std::string & file, int line, size_t column) :
        StatementNode(file, line, column),
        targetName_(std::move(targetName)),
        propertyPath_(std::move(propertyPath)),
        rhs_(std::move(rhs)) {}

    void interpret(Interpreter & interpreter) const override {
        using namespace Symbols;
        auto * symContainer = SymbolContainer::instance();

        // Find the target symbol hierarchically
        auto symbol = symContainer->findSymbol(targetName_);

        if (!symbol) {
            // Use the error message from findSymbol which indicates search scope
            throw Exception(
                "Variable '" + targetName_ + "' not found starting from scope: " + symContainer->currentScopeName(),
                filename_, line_, column_);
        }

        // Check if the found symbol is a constant
        if (symbol->getKind() == Kind::Constant) {
            throw Exception("Cannot assign to constant '" + targetName_ + "'", filename_, line_, column_);
        }

        // If propertyPath_ is not empty, we are assigning to an object member
        if (!propertyPath_.empty()) {
            // Get the object value from the base symbol
            Symbols::ValuePtr objectValue = symbol->getValue();
            if (objectValue != Variables::Type::OBJECT && objectValue != Variables::Type::CLASS) {
                throw Exception("Attempting to assign property on non-object variable '" + targetName_ + "'", filename_,
                                line_, column_);
            }

            // Evaluate RHS first
            Symbols::ValuePtr newValue = rhs_->evaluate(interpreter);

            // Traverse and modify the nested object structure IN PLACE within the Value obtained from the symbol
            // (Requires Value to allow modification of its internal map, or handle copy-on-write)
            // Assuming Value allows modification via get() returning a reference or similar mechanism
            // --- Modification Logic (Needs careful review based on Value implementation) ---
            auto currentVal = objectValue;  // Start with the base object value
            for (size_t i = 0; i < propertyPath_.size(); ++i) {
                const auto & key = propertyPath_[i];
                if (currentVal != Variables::Type::OBJECT && currentVal != Variables::Type::CLASS) {
                    throw Exception("Intermediate property '" + key + "' is not an object", filename_, line_, column_);
                }

                Symbols::ObjectMap currentMap = currentVal;  // Get reference to map
                auto               it         = currentMap.find(key);
                if (it == currentMap.end()) {
                    throw Exception("Property '" + key + "' not found on object", filename_, line_, column_);
                }

                if (i == propertyPath_.size() - 1) {
                    // Last element: perform assignment
                    // Type check before assignment
                    if (newValue != Variables::Type::NULL_TYPE && it->second != Variables::Type::NULL_TYPE &&
                        newValue != it->second) {
                        throw Exception("Type mismatch for property '" + key + "': expected '" +
                                            Symbols::Variables::TypeToString(it->second) + "' but got '" +
                                            Symbols::Variables::TypeToString(newValue) + "'",
                                        filename_, line_, column_);
                    }
                    it->second = newValue;  // Assign the new value to the property in the map
                } else {
                    // Move to the next level
                    currentVal = it->second;
                }
            }
            // --- End Modification Logic ---

            // Update the original symbol with the modified objectValue
            symbol->setValue(objectValue);

        } else {
            // Simple variable assignment (targetName_ is the variable itself)
            auto newValue     = rhs_->evaluate(interpreter);
            auto currentValue = symbol->getValue();  // Get current value for type checking

            // Type check (allow assigning NULL to anything, otherwise types must match)
            if (newValue != Variables::Type::NULL_TYPE && currentValue != Variables::Type::NULL_TYPE &&
                newValue != currentValue) {
                throw Exception("Type mismatch assigning to variable '" + targetName_ + "': expected '" +
                                    Symbols::Variables::TypeToString(currentValue) + "' but got '" +
                                    Symbols::Variables::TypeToString(newValue) + "'",
                                filename_, line_, column_);
            }
            symbol->setValue(newValue);  // Assign directly to the symbol
        }
    }

    std::string toString() const override {
        std::string repr = "Assignment: " + targetName_;
        for (const auto & key : propertyPath_) {
            repr += "->" + key;
        }
        return repr;
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP
