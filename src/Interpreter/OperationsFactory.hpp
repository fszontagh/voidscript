#ifndef OPERATIONSFACTORY_HPP
#define OPERATIONSFACTORY_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionBuilder.hpp"
#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Nodes/Expression/LiteralExpressionNode.hpp"
#include "Nodes/Statement/CallStatementNode.hpp"
#include "Nodes/Statement/DeclareFunctionStatementNode.hpp"
#include "Nodes/Statement/DeclareVariableStatementNode.hpp"
#include "Nodes/Statement/ExpressionStatementNode.hpp"
#include "Nodes/Statement/ReturnStatementNode.hpp"
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

    static void defineSimpleVariable(const std::string & varName, Symbols::Value::ValuePtr value,
                                     const std::string & ns, const std::string & filename, int line, size_t column) {
        auto literalExpr = std::make_unique<LiteralExpressionNode>(value);

        std::unique_ptr<DeclareVariableStatementNode> stmt = std::make_unique<DeclareVariableStatementNode>(
            varName, ns, value->getType(), std::move(literalExpr), filename, line, column);

        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Declaration, varName, std::move(stmt) });
    }

    static void defineSimpleConstantVariable(const std::string & varName, Symbols::Value::ValuePtr value,
                                             const std::string & ns, const std::string & filename, int line,
                                             size_t column) {
        auto literalExpr = std::make_unique<LiteralExpressionNode>(value);

        std::unique_ptr<DeclareVariableStatementNode> stmt = std::make_unique<DeclareVariableStatementNode>(
            varName, ns, value->getType(), std::move(literalExpr), filename, line, column, /*isConst*/ true);

        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Declaration, varName, std::move(stmt) });
    }

    static void defineVariableWithExpression(const std::string & varName, Symbols::Variables::Type type,
                                             const Parser::ParsedExpressionPtr & pexpr, const std::string & ns,
                                             const std::string & filename, int line, size_t column) {
        // ParsedExpression → ExpressionNode
        std::unique_ptr<ExpressionNode> expr = buildExpressionFromParsed(pexpr);

        std::unique_ptr<DeclareVariableStatementNode> stmt =
            std::make_unique<DeclareVariableStatementNode>(varName, ns, type, std::move(expr), filename, line, column);

        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Declaration, varName, std::move(stmt) });
    }

    /**
     * @brief Record a constant declaration operation with an initializer expression.
     */
    static void defineConstantWithExpression(const std::string & varName, Symbols::Variables::Type type,
                                             const Parser::ParsedExpressionPtr & pexpr, const std::string & ns,
                                             const std::string & filename, int line, size_t column) {
        // Build initializer expression
        std::unique_ptr<ExpressionNode>               expr = buildExpressionFromParsed(pexpr);
        // Create declaration node with const flag
        std::unique_ptr<DeclareVariableStatementNode> stmt = std::make_unique<DeclareVariableStatementNode>(
            varName, ns, type, std::move(expr), filename, line, column, /* isConst */ true);
        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Declaration, varName, std::move(stmt) });
    }

    /**
     * @brief Record a function call operation with argument expressions.
     * @param functionName Name of the function being called.
     * @param parsedArgs Vector of parsed argument expressions.
     * @param ns Current namespace scope for operations.
     * @param fileName Source filename.
     * @param line Line number of call.
     * @param column Column number of call.
     */
    static void callFunction(const std::string & functionName, std::vector<Parser::ParsedExpressionPtr> && parsedArgs,
                             const std::string & ns, const std::string & fileName, int line, size_t column) {
        // Build argument ExpressionNode list
        std::vector<std::unique_ptr<ExpressionNode>> exprs;
        exprs.reserve(parsedArgs.size());
        for (auto & pexpr : parsedArgs) {
            exprs.push_back(buildExpressionFromParsed(pexpr));
        }
        // Create call statement node
        auto stmt = std::make_unique<CallStatementNode>(functionName, std::move(exprs), fileName, line, column);
        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::FunctionCall, functionName, std::move(stmt) });
    }

    /**
     * @brief Record a return statement operation inside a function.
     * @param pexpr Parsed expression for return value, or nullptr for void return.
     * @param ns Current namespace (function scope).
     * @param fileName Source filename.
     * @param line Line number of return.
     * @param column Column number of return.
     */
    static void callReturn(const Parser::ParsedExpressionPtr & pexpr, const std::string & ns,
                           const std::string & fileName, int line, size_t column) {
        std::unique_ptr<ExpressionNode> expr = pexpr ? buildExpressionFromParsed(pexpr) : nullptr;
        auto stmt = std::make_unique<ReturnStatementNode>(std::move(expr), fileName, line, column);
        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Return, std::string(), std::move(stmt) });
    }

    /**
     * @brief Record a generic expression statement (e.g., method call).
     * @param pexpr Parsed expression for evaluation.
     * @param ns    Current namespace scope for operations.
     * @param fileName Source filename.
     * @param line  Line number of statement.
     * @param column Column number of statement.
     */
    static void callExpression(const Parser::ParsedExpressionPtr & pexpr, const std::string & ns,
                               const std::string & fileName, int line, size_t column) {
        std::unique_ptr<ExpressionNode> expr = buildExpressionFromParsed(pexpr);
        auto stmt = std::make_unique<ExpressionStatementNode>(std::move(expr), fileName, line, column);
        Operations::Container::instance()->add(
            ns, Operations::Operation{ Operations::Type::Expression, std::string(), std::move(stmt) });
    }
};

}  // namespace Interpreter

#endif  // OPERATIONSFACTORY_HPP
