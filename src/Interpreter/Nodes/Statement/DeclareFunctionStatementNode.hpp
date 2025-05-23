#ifndef INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP
#define INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <utility>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"

namespace Interpreter {

class DeclareFunctionStatementNode : public StatementNode {
    std::string                     functionName_;
    Symbols::Variables::Type        returnType_;
    Symbols::FunctionParameterInfo  params_;
    std::unique_ptr<ExpressionNode> expression_;
    std::string                     ns;


  public:
    DeclareFunctionStatementNode(const std::string & function_name, const std::string & ns,
                                 const Symbols::FunctionParameterInfo & params, Symbols::Variables::Type return_type,
                                 std::unique_ptr<ExpressionNode> expr, const std::string & file_name, int file_line,
                                 size_t line_column) :
        StatementNode(file_name, file_line, line_column),
        functionName_(function_name),
        returnType_(return_type),
        params_(params),
        expression_(std::move(expr)),
        ns(ns) {}

    void interpret(Interpreter & /*interpreter*/) const override {
        try {
            auto *sc = Symbols::SymbolContainer::instance();
            auto targetTable = sc->getScopeTable(ns); // 'ns' is the current scope for this function

            if (!targetTable) {
                 // This case should ideally be prevented by parser ensuring scope exists before defining function in it.
                throw Exception("Target scope '" + ns + "' for function declaration does not exist", filename_, line_, column_);
            }

            // Check if function already declared in the target scope's function namespace
            if (targetTable->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, functionName_)) {
                throw Exception("Function '" + functionName_ + "' already declared in scope '" + ns + "'", filename_, line_, column_);
            }

            // If we are here, the function is not yet defined in the current scope's function namespace.
            // The old check Symbols::SymbolContainer::instance()->exists(functionName_) was too broad and could lead to incorrect shadowing issues or missed re-declarations.

            const auto func = Symbols::SymbolFactory::createFunction(functionName_, ns, params_, "", returnType_);
            std::cout << "Defining function: " << func->name() << ": "<< ns << std::endl;
            sc->defineInScope(ns, func); // Explicitly define in the target scope 'ns'

        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return std::string(" FunctioName: " + functionName_ +
                           " return type: " + Symbols::Variables::TypeToString(returnType_) +
                           " params size: " + std::to_string(params_.size()));
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP
