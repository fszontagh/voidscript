#ifndef INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP
#define INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP

#include "StatementNode.hpp"
#include "ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node for assignments: variable or nested object property.
 *   e.g., $a = expr; or $obj->prop->sub = expr;
 */
class AssignmentStatementNode : public StatementNode {
  private:
    std::string targetName_;
    std::vector<std::string> propertyPath_;
    std::unique_ptr<ExpressionNode> rhs_;
  public:
    AssignmentStatementNode(std::string targetName,
                            std::vector<std::string> propertyPath,
                            std::unique_ptr<ExpressionNode> rhs,
                            const std::string & file,
                            int line,
                            size_t column)
        : StatementNode(file, line, column),
          targetName_(std::move(targetName)),
          propertyPath_(std::move(propertyPath)),
          rhs_(std::move(rhs)) {}

    void interpret(Interpreter & interpreter) const override {
        using namespace Symbols;
        auto * symContainer = SymbolContainer::instance();
        // Variables are stored under <scope>.variables
        const std::string base_ns = symContainer->currentScopeName();
        const std::string var_ns = base_ns + ".variables";
        if (!symContainer->exists(targetName_, var_ns)) {
            throw std::runtime_error("Variable '" + targetName_ + "' does not exist in namespace: " + var_ns +
                                     " File: " + filename_ + ", Line: " + std::to_string(line_) +
                                     ", Column: " + std::to_string(column_));
        }
        auto symbol = symContainer->get(var_ns, targetName_);
        // Copy current value for potential nested updates
        Value varValue = symbol->getValue();
        // Evaluate RHS
        Value newValue = rhs_->evaluate(interpreter);
        // Simple variable assignment
        if (propertyPath_.empty()) {
            // Type check
            if (newValue.getType() != varValue.getType()) {
                using namespace Variables;
                throw std::runtime_error("Type mismatch assigning to variable '" + targetName_ +
                                         "': expected '" + TypeToString(varValue.getType()) +
                                         "' but got '" + TypeToString(newValue.getType()) +
                                         "' File: " + filename_ + ", Line: " + std::to_string(line_) +
                                         ", Column: " + std::to_string(column_));
            }
            symbol->setValue(newValue);
            return;
        }
        // Nested object property assignment
        if (varValue.getType() != Variables::Type::OBJECT) {
            throw std::runtime_error("Attempting to assign property on non-object variable '" + targetName_ + "'");
        }
        // Traverse into nested maps
        using ObjectMap = Value::ObjectMap;
        ObjectMap * currMap = &std::get<ObjectMap>(varValue.get());
        // Iterate through all but last key
        for (size_t i = 0; i + 1 < propertyPath_.size(); ++i) {
            const auto & key = propertyPath_[i];
            auto it = currMap->find(key);
            if (it == currMap->end()) {
                throw std::runtime_error("Property '" + key + "' not found on object '" + targetName_ + "'");
            }
            Value & child = it->second;
            if (child.getType() != Variables::Type::OBJECT) {
                throw std::runtime_error("Property '" + key + "' is not an object, cannot assign nested property");
            }
            currMap = &std::get<ObjectMap>(child.get());
        }
        // Last key
        const std::string & lastKey = propertyPath_.back();
        auto it = currMap->find(lastKey);
        if (it == currMap->end()) {
            throw std::runtime_error("Property '" + lastKey + "' not found on object '" + targetName_ + "'");
        }
        // Type check against existing property
        if (newValue.getType() != it->second.getType()) {
            using namespace Variables;
            throw std::runtime_error("Type mismatch for property '" + lastKey + "': expected '" +
                                     TypeToString(it->second.getType()) + "' but got '" +
                                     TypeToString(newValue.getType()) + "' File: " + filename_ +
                                     ", Line: " + std::to_string(line_) + ", Column: " + std::to_string(column_));
        }
        // Assign and write back to symbol
        (*currMap)[lastKey] = newValue;
        symbol->setValue(varValue);
    }

    std::string toString() const override {
        std::string repr = "Assignment: " + targetName_;
        for (const auto & key : propertyPath_) repr += "->" + key;
        return repr;
    }
};

} // namespace Interpreter

#endif // INTERPRETER_ASSIGNMENT_STATEMENT_NODE_HPP