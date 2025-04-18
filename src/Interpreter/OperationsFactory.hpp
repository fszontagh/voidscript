#ifndef OPERATIONSFACTORY_HPP
#define OPERATIONSFACTORY_HPP

#include <memory>
#include <string>

#include "Interpreter/DeclareFunctionStatementNode.hpp"
#include "Interpreter/DeclareVariableStatementNode.hpp"
#include "Interpreter/ExpressionBuilder.hpp"
#include "Interpreter/LiteralExpressionNode.hpp"
#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class OperationsFactory {
  public:
    OperationsFactory() {}

    static void defineFunction(const std::string & functionName, const Symbols::FunctionParameterInfo & params,
                               const Symbols::Variables::Type & returnType, const std::string & ns,
                               const std::string & fileName, int line, size_t column) {
        std::unique_ptr<DeclareFunctionStatementNode> stmt = std::make_unique<DeclareFunctionStatementNode>(
            functionName, ns, params, returnType, nullptr, fileName, line, column);
        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::FuncDeclaration, functionName, std::move(stmt) });
    }

    static void defineSimpleVariable(const std::string & varName, const Symbols::Value & value, const std::string & ns,
                                     const std::string & filename, int line, size_t column) {
        auto literalExpr = std::make_unique<LiteralExpressionNode>(value);

        std::unique_ptr<DeclareVariableStatementNode> stmt = std::make_unique<DeclareVariableStatementNode>(
            varName, ns, value.getType(), std::move(literalExpr), filename, line, column);

        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Declaration, varName, std::move(stmt) });
    }

    static void defineVariableWithExpression(const std::string & varName, Symbols::Variables::Type type,
                                             const Parser::ParsedExpressionPtr pexpr, const std::string & ns,
                                             const std::string & filename, int line, size_t column) {
                                              // ParsedExpression → ExpressionNode
                                              std::unique_ptr<ExpressionNode> expr = buildExpressionFromParsed(pexpr);

                                              std::unique_ptr<DeclareVariableStatementNode> stmt = std::make_unique<DeclareVariableStatementNode>(
                                                  varName, ns, type, std::move(expr), filename, line, column);

                                              Operations::Container::instance()->add(
                                                  ns, Operations::Operation{Operations::Type::Declaration, varName, std::move(stmt)});
                                          }
    
    /**
     * @brief Record a function call operation for later detection.
     * @param functionName Name of the function being called.
     * @param ns Current namespace scope.
     * @param fileName Source filename.
     * @param line Line number of call.
     * @param column Column number of call.
     */
    static void callFunction(const std::string & functionName,
                             const std::string & ns,
                             const std::string & fileName,
                             int line,
                             size_t column) {
        // No associated StatementNode; this is for detection only
        Operations::Container::instance()->add(
            ns,
            Operations::Operation{Operations::Type::FunctionCall, functionName, nullptr});
    }
};

}  // namespace Interpreter

#endif  // OPERATIONSFACTORY_HPP
