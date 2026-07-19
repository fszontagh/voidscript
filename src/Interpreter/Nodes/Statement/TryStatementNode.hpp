#ifndef INTERPRETER_TRY_STATEMENT_NODE_HPP
#define INTERPRETER_TRY_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/BreakException.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Interpreter/ThrowException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief try { ... } catch { ... } and try { ... } catch (string $e) { ... }
 *
 * Catches script-level `throw` and built-in runtime errors alike, so a script can
 * recover from e.g. a bad conversion instead of the whole process dying.
 *
 * Control flow is NOT catchable. BreakException derives from std::runtime_error, so
 * without an explicit rethrow a `break` inside a try would be swallowed and turn into
 * an infinite loop. ReturnException does not derive from std::exception at all, so it
 * passes through on its own, but it is rethrown explicitly to keep that intent visible
 * rather than dependent on a base-class detail.
 */
class TryStatementNode : public StatementNode {
  private:
    std::vector<std::unique_ptr<StatementNode>> tryBody_;
    std::vector<std::unique_ptr<StatementNode>> catchBody_;
    std::string                                 catchVarName_;  // empty when `catch { }`

  public:
    TryStatementNode(std::vector<std::unique_ptr<StatementNode>> tryBody,
                     std::vector<std::unique_ptr<StatementNode>> catchBody, std::string catchVarName,
                     const std::string & file, int line, size_t column) :
        StatementNode(file, line, column),
        tryBody_(std::move(tryBody)),
        catchBody_(std::move(catchBody)),
        catchVarName_(std::move(catchVarName)) {}

    void interpret(Interpreter & interpreter) const override {
        Symbols::ValuePtr caughtValue;
        bool              caught = false;

        try {
            for (const auto & stmt : tryBody_) {
                stmt->interpret(interpreter);
            }
        } catch (const ReturnException &) {
            throw;  // control flow, not an error
        } catch (const BreakException &) {
            throw;  // control flow, and it IS a std::exception - must precede the catch below
        } catch (const ThrowException & e) {
            caughtValue = e.value();
            caught      = true;
        } catch (const std::exception & e) {
            // Built-in runtime errors surface to the script as their message text.
            caughtValue = Symbols::ValuePtr(std::string(e.what()));
            caught      = true;
        }

        if (!caught) {
            return;
        }

        if (!catchVarName_.empty()) {
            auto * sc = Symbols::SymbolContainer::instance();
            auto   sym =
                Symbols::SymbolFactory::createVariable(catchVarName_, caughtValue, sc->currentScopeName());
            sc->addVariable(sym);
        }

        for (const auto & stmt : catchBody_) {
            stmt->interpret(interpreter);
        }
    }

    std::string toString() const override {
        return "TryStatementNode{ catchVar='" + catchVarName_ + "' }";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_TRY_STATEMENT_NODE_HPP
