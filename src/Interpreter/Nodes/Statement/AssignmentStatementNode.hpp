#ifndef INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP
#define INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP

#include <iostream> // For std::cerr
#include <string>   // For std::string
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

        // +++ Add New Logging +++
        std::string full_target_path = targetName_;
        for(const auto& p : propertyPath_) full_target_path += "->" + p;
        // +++ End New Logging +++

        // First try to get the variable (most common case)
        auto symbol = symContainer->getVariable(targetName_);

        // If not found, try to get it as a constant (which will fail for assignment later)
        if (!symbol) {
            symbol = symContainer->getConstant(targetName_);
        }

        if (!symbol) {
            auto symbol = SymbolFactory::createConstant(targetName_, Symbols::ValuePtr::undefined(),
                                                        symContainer->currentScopeName());
            symContainer->addConstant(symbol);

            //throw Exception(
            //    "Variable '" + targetName_ + "' not found starting from scope: " + symContainer->currentScopeName(),
            //    filename_, line_, column_);
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
            // Symbols::ValuePtr newValue = rhs_->evaluate(interpreter); // Evaluate only once before loop
            Symbols::ValuePtr newValueRhs = rhs_->evaluate(interpreter);

            // Traverse and modify the nested object structure IN PLACE.
            Symbols::ValuePtr currentValPtr = objectValue;

            for (size_t i = 0; i < propertyPath_.size(); ++i) {
                const auto & key = propertyPath_[i];

                if (currentValPtr.getType() != Variables::Type::OBJECT &&
                    currentValPtr.getType() != Variables::Type::CLASS) {
                    std::string traversed_path;
                    for (size_t j = 0; j < i; ++j) {
                        traversed_path += propertyPath_[j] + "->";
                    }
                    throw Exception(
                        "Attempting to access property '" + key + "' on non-object type at path '" + targetName_ +
                            "->" + traversed_path.substr(0, traversed_path.length() - 2) +
                            "'. Current segment type: " + Symbols::Variables::TypeToString(currentValPtr.getType()),
                        filename_, line_, column_);
                }

                // Get a reference to the map within the current Value object.
                // Value::get<ObjectMap>() returns ObjectMap&
                Symbols::ObjectMap & map_ref = currentValPtr->get<Symbols::ObjectMap>();

                if (i == propertyPath_.size() - 1) {
                    // This is the final property to assign.
                    // Symbols::ValuePtr newValueEvaluated = rhs_->evaluate(interpreter); // Already evaluated as newValueRhs

                    // Optional Type Check:
                    if (map_ref.count(key)) {
                        const Symbols::ValuePtr & existing_prop_val = map_ref.at(key);
                        if (newValueRhs.getType() != Symbols::Variables::Type::NULL_TYPE && // Use newValueRhs
                            existing_prop_val.getType() != Symbols::Variables::Type::NULL_TYPE &&
                            newValueRhs.getType() != existing_prop_val.getType()) { // Use newValueRhs
                            throw Exception("Type mismatch for property '" + key + "': expected '" +
                                                Symbols::Variables::TypeToString(existing_prop_val.getType()) +
                                                "' but got '" +
                                                Symbols::Variables::TypeToString(newValueRhs.getType()) + "'", // Use newValueRhs
                                            filename_, line_, column_);
                        }
                    }
                    // Only reference assignment, do not clone ValuePtr
                    // map_ref[key] = newValueEvaluated;
                    map_ref[key] = newValueRhs; // Use the already evaluated newValueRhs
                    if (targetName_ == "this") {
                        // Ensure 'this' object and key exist before trying to access for verification
                        const auto& thisObj = interpreter.getThisObject();
                        if (thisObj->getType() != Symbols::Variables::Type::NULL_TYPE && !thisObj->isNULL() && map_ref.count(key)) {
                        }
                    }
                } else {
                    // Not the last property, so traverse deeper.
                    auto it = map_ref.find(key);
                    if (it == map_ref.end()) {
                        std::string traversed_path;
                        for (size_t j = 0; j <= i; ++j) {
                            traversed_path += propertyPath_[j] + (j < i ? "->" : "");
                        }
                        throw Exception("Property '" + key + "' not found on object at path '" + targetName_ + "->" +
                                            traversed_path + "'",
                                        filename_, line_, column_);
                    }
                    currentValPtr = it->second;  // Update currentValPtr to the next ValuePtr in the chain.
                }
            }
            // No need for symbol->setValue(objectValue) as modifications are direct.

        } else {
            // Simple variable assignment (targetName_ is the variable itself)
            Symbols::ValuePtr newValue = rhs_->evaluate(interpreter);
            auto currentValue = symbol->getValue();  // Get current value for type checking

            // Type check (allow assigning NULL to anything, otherwise types must match)
            if (newValue != Variables::Type::NULL_TYPE && currentValue != Variables::Type::NULL_TYPE &&
                newValue.getType() != currentValue.getType()) { // Corrected type comparison
                throw Exception("Type mismatch assigning to variable '" + targetName_ + "': expected '" +
                                    Symbols::Variables::TypeToString(currentValue.getType()) + "' but got '" +
                                    Symbols::Variables::TypeToString(newValue.getType()) + "'",
                                filename_, line_, column_);
            }
            // Only reference assignment, do not clone ValuePtr
            symbol->setValue(newValue);
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
