#ifndef INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP
#define INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <utility>

#include "ExpressionNode.hpp"
#include "Interpreter.hpp"
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
        //Symbols::Value value = expression_->evaluate(interpreter);
        if (Symbols::SymbolContainer::instance()->exists(functionName_)) {
            throw std::runtime_error("Function already declared: " + functionName_ + " file: " + filename_ +
                                     ", line: " + std::to_string(line_) + ", column: " + std::to_string(column_));
        }
        const auto func = Symbols::SymbolFactory::createFunction(functionName_, ns, params_, "", returnType_);
        Symbols::SymbolContainer::instance()->add(func);
    }

    std::string toString() const override {
        return std::string( " FunctioName: " + functionName_ + " return type: " + Symbols::Variables::TypeToString(returnType_) +
               " params size: " + std::to_string(params_.size()));
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_DEFINE_FUNCTION_STATEMENT_NODE_HPP
