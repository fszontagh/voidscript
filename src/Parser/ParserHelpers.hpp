#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Interpreter/Nodes/Statement/AssignmentStatementNode.hpp"
#include "Interpreter/Nodes/Expression/IdentifierExpressionNode.hpp"
#include "Interpreter/Nodes/Expression/LiteralExpressionNode.hpp"
#include "Interpreter/Nodes/Expression/BinaryExpressionNode.hpp"
#include "Symbols/Value.hpp"
#include "Lexer/Token.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Parser/Parser.hpp"

namespace ParserHelpers {

// Removes a leading '$' if present
inline std::string stripDollarPrefix(const std::string& name) {
    return (!name.empty() && name[0] == '$') ? name.substr(1) : name;
}

// Builds a ++/-- assignment statement node (for both prefix/postfix uses)
inline std::unique_ptr<Interpreter::StatementNode> buildIncDecAssignmentNode(
    const std::string &varName, const std::string &op, const std::string &filename, int line, int col) {
    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(varName);
    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(std::make_shared<Symbols::Value>(1));
    auto binOp = std::make_unique<Interpreter::BinaryExpressionNode>(
        std::move(lhs), op, std::move(rhs));
    return std::make_unique<Interpreter::AssignmentStatementNode>(
        varName, std::vector<std::string>(), std::move(binOp), filename, line, col);
}

// Checks if the token is a built-in variable type, or a registered class id
inline bool isTypeOrClassToken(const Lexer::Tokens::Token &tok) {
    if (Parser::Parser::variable_types.find(tok.type) != Parser::Parser::variable_types.end()) return true;
    if (tok.type == Lexer::Tokens::Type::IDENTIFIER &&
        Modules::UnifiedModuleManager::instance().hasClass(tok.value)) return true;
    return false;
}

} // namespace ParserHelpers