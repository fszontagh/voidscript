#ifndef INTERPRETER_INDEXED_ASSIGNMENT_STATEMENT_NODE_HPP
#define INTERPRETER_INDEXED_ASSIGNMENT_STATEMENT_NODE_HPP

#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node for assignment into an indexed container:
 *   $a[0] = expr;  $q->outputs[1] = expr;  $d->rows[1][0] = expr;
 *
 * Arrays are ObjectMaps keyed by the decimal index ("0", "1", ...), so this is the
 * same operation as an object property assignment, just with a key computed at
 * runtime. The container expression is evaluated to a ValuePtr, which is shared
 * rather than cloned, so writing through the returned map mutates the original.
 */
class IndexedAssignmentStatementNode : public StatementNode {
  private:
    std::unique_ptr<ExpressionNode> containerExpr_;
    std::unique_ptr<ExpressionNode> indexExpr_;
    std::unique_ptr<ExpressionNode> rhs_;

  public:
    IndexedAssignmentStatementNode(std::unique_ptr<ExpressionNode> containerExpr,
                                   std::unique_ptr<ExpressionNode> indexExpr, std::unique_ptr<ExpressionNode> rhs,
                                   const std::string & file, int line, size_t column) :
        StatementNode(file, line, column),
        containerExpr_(std::move(containerExpr)),
        indexExpr_(std::move(indexExpr)),
        rhs_(std::move(rhs)) {}

    void interpret(Interpreter & interpreter) const override {
        using namespace Symbols;

        ValuePtr container = containerExpr_->evaluate(interpreter, filename_, line_, column_);
        if (container != Variables::Type::OBJECT && container != Variables::Type::CLASS) {
            throw Exception("Attempted to assign to an index of a non-array value", filename_, line_, column_);
        }

        ObjectMap & map_ref = container->get<ObjectMap>();

        std::string key;
        if (!indexExpr_) {
            // Append: `$a[] = expr`. Arrays are keyed by consecutive decimal indices,
            // so the next free slot is the current size.
            key = std::to_string(map_ref.size());
        } else {
            ValuePtr idxVal = indexExpr_->evaluate(interpreter, filename_, line_, column_);
            if (idxVal == Variables::Type::INTEGER) {
                key = std::to_string(idxVal->get<int>());
            } else if (idxVal == Variables::Type::STRING) {
                key = idxVal.get<std::string>();
            } else {
                throw Exception("Array index must be integer or string, got '" +
                                    Variables::TypeToString(idxVal.getType()) + "'",
                                filename_, line_, column_);
            }
        }

        ValuePtr newValue = rhs_->evaluate(interpreter, filename_, line_, column_);

        // Match the element type check AssignmentStatementNode does for properties:
        // replacing an existing element must not silently change its type.
        auto existing = map_ref.find(key);
        if (existing != map_ref.end() && newValue.getType() != Variables::Type::NULL_TYPE &&
            existing->second.getType() != Variables::Type::NULL_TYPE &&
            newValue.getType() != existing->second.getType()) {
            throw Exception("Type mismatch for element '" + key + "': expected '" +
                                Variables::TypeToString(existing->second.getType()) + "' but got '" +
                                Variables::TypeToString(newValue.getType()) + "'",
                            filename_, line_, column_);
        }

        map_ref[key] = newValue;
    }

    std::string toString() const override {
        return "IndexedAssignmentStatementNode{ " + containerExpr_->toString() + "[" + indexExpr_->toString() + "] }";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_INDEXED_ASSIGNMENT_STATEMENT_NODE_HPP
