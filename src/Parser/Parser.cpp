#include "Parser/Parser.hpp"
// Static filename for unified error reporting in Parser::Exception
std::string Parser::Parser::Exception::current_filename_;

#include <stack>

#include "Interpreter/ExpressionBuilder.hpp"
#include "Interpreter/Nodes/Statement/AssignmentStatementNode.hpp"
#include "Interpreter/Nodes/Statement/CallStatementNode.hpp"
#include "Interpreter/Nodes/Statement/ClassDefinitionStatementNode.hpp"
#include "Interpreter/Nodes/Statement/ConditionalStatementNode.hpp"
#include "Interpreter/Nodes/Statement/CStyleForStatementNode.hpp"
#include "Interpreter/Nodes/Statement/DeclareVariableStatementNode.hpp"
#include "Interpreter/Nodes/Statement/ExpressionStatementNode.hpp"
#include "Interpreter/Nodes/Statement/ForStatementNode.hpp"
#include "Interpreter/Nodes/Statement/ReturnStatementNode.hpp"
#include "Interpreter/Nodes/Statement/WhileStatementNode.hpp"
#include "Interpreter/OperationsFactory.hpp"
#include "Lexer/Operators.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/SymbolContainer.hpp"

// Additional necessary includes, if needed
namespace Parser {

const std::unordered_map<std::string, Lexer::Tokens::Type> Parser::keywords = {
    { "if",       Lexer::Tokens::Type::KEYWORD_IF                   },
    { "else",     Lexer::Tokens::Type::KEYWORD_ELSE                 },
    { "while",    Lexer::Tokens::Type::KEYWORD_WHILE                },
    { "for",      Lexer::Tokens::Type::KEYWORD_FOR                  },
    { "return",   Lexer::Tokens::Type::KEYWORD_RETURN               },
    { "function", Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION },
    // Older keywords:
    { "const",    Lexer::Tokens::Type::KEYWORD_CONST                },
    // Class support keywords
    { "class",    Lexer::Tokens::Type::KEYWORD_CLASS                },
    { "private",  Lexer::Tokens::Type::KEYWORD_PRIVATE              },
    { "public",   Lexer::Tokens::Type::KEYWORD_PUBLIC               },
    { "new",      Lexer::Tokens::Type::KEYWORD_NEW                  },
    { "true",     Lexer::Tokens::Type::KEYWORD                      },
    { "false",    Lexer::Tokens::Type::KEYWORD                      },
    // variable types
    { "null",     Lexer::Tokens::Type::KEYWORD_NULL                 },
    { "int",      Lexer::Tokens::Type::KEYWORD_INT                  },
    { "double",   Lexer::Tokens::Type::KEYWORD_DOUBLE               },
    { "float",    Lexer::Tokens::Type::KEYWORD_FLOAT                },
    { "string",   Lexer::Tokens::Type::KEYWORD_STRING               },
    { "boolean",  Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "bool",     Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "object",   Lexer::Tokens::Type::KEYWORD_OBJECT               },
    // ... other keywords ...
};

const std::unordered_map<Lexer::Tokens::Type, Symbols::Variables::Type> Parser::variable_types = {
    { Lexer::Tokens::Type::KEYWORD_INT,     Symbols::Variables::Type::INTEGER   },
    { Lexer::Tokens::Type::KEYWORD_DOUBLE,  Symbols::Variables::Type::DOUBLE    },
    { Lexer::Tokens::Type::KEYWORD_FLOAT,   Symbols::Variables::Type::FLOAT     },
    { Lexer::Tokens::Type::KEYWORD_STRING,  Symbols::Variables::Type::STRING    },
    { Lexer::Tokens::Type::KEYWORD_NULL,    Symbols::Variables::Type::NULL_TYPE },
    { Lexer::Tokens::Type::KEYWORD_BOOLEAN, Symbols::Variables::Type::BOOLEAN   },
    { Lexer::Tokens::Type::KEYWORD_OBJECT,  Symbols::Variables::Type::OBJECT    },
};

// Parse a top-level constant variable definition: const <type> $name = expr;
void Parser::parseConstVariableDefinition() {
    // 'const'
    auto                     constTok = expect(Lexer::Tokens::Type::KEYWORD_CONST, "const");
    // Parse type
    Symbols::Variables::Type var_type = parseType();
    // Variable name
    Lexer::Tokens::Token     id_token;
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        id_token = consumeToken();
    } else {
        reportError("Expected variable name after 'const'", currentToken());
    }
    std::string var_name = id_token.value;
    if (!var_name.empty() && var_name[0] == '$') {
        var_name = var_name.substr(1);
    }
    const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();
    // Expect assignment
    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
    // Parse initializer expression
    auto expr = parseParsedExpression(var_type);
    // Record constant definition
    Interpreter::OperationsFactory::defineConstantWithExpression(var_name, var_type, expr, ns, current_filename_,
                                                                 id_token.line_number, id_token.column_number);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
}

void Parser::parseVariableDefinition() {
    Symbols::Variables::Type var_type = parseType();

    // Variable name: allow names with or without leading '$'
    Lexer::Tokens::Token id_token;
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        id_token = consumeToken();
    } else {
        reportError("Expected variable name", currentToken());
    }
    std::string var_name = id_token.value;

    if (!var_name.empty() && var_name[0] == '$') {
        var_name = var_name.substr(1);
    }
    const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();

    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");

    auto expr = parseParsedExpression(var_type);
    Interpreter::OperationsFactory::defineVariableWithExpression(var_name, var_type, expr, ns, current_filename_,
                                                                 id_token.line_number, id_token.column_number);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
}

// Parse a top-level assignment statement and record it
void Parser::parseAssignmentStatement() {
    // Special case for 'this' keyword
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "this") {
        // Consume 'this' keyword
        auto thisTok = consumeToken();
        
        // Expect property access
        expect(Lexer::Tokens::Type::PUNCTUATION, "->");
        
        // Get property name (with or without $)
        Lexer::Tokens::Token propTok;
        if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
            currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
            propTok = consumeToken();
        } else {
            reportError("Expected property name after 'this->'");
        }
        
        std::string propName = propTok.value;
        if (!propName.empty() && propName[0] == '$') {
            propName = propName.substr(1);
        }
        
        std::vector<std::string> propertyPath = { propName };
        
        // Handle additional property access (obj->prop1->prop2->...)
        while (match(Lexer::Tokens::Type::PUNCTUATION, "->")) {
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                auto nextPropTok = consumeToken();
                std::string nextPropName = nextPropTok.value;
                if (!nextPropName.empty() && nextPropName[0] == '$') {
                    nextPropName = nextPropName.substr(1);
                }
                propertyPath.push_back(nextPropName);
            } else {
                reportError("Expected property name after '->'");
            }
        }
        
        // Expect assignment operator
        auto opTok = expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);
        
        // Parse RHS expression
        auto rhsExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        
        // Expect closing semicolon
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        
        // Build RHS node
        auto rhsNode = buildExpressionFromParsed(rhsExpr);
        
        // Handle compound assignment operators (+=, -=, etc.)
        if (opTok.value != "=") {
            std::unique_ptr<Interpreter::ExpressionNode> lhsNode =
                std::make_unique<Interpreter::IdentifierExpressionNode>("this");
            
            // Build property access chain
            for (const auto & prop : propertyPath) {
                auto member = std::make_unique<Interpreter::MemberExpressionNode>(
                    std::move(lhsNode), prop, this->current_filename_, 
                    thisTok.line_number, thisTok.column_number);
                lhsNode = std::move(member);
            }
            
            // Extract operator without '='
            std::string binOp = opTok.value.substr(0, opTok.value.size() - 1);
            
            // Create binary operation: this->prop OP rhs
            rhsNode = std::make_unique<Interpreter::BinaryExpressionNode>(
                std::move(lhsNode), binOp, std::move(rhsNode));
        }
        
        // Create assignment operation
        auto assignNode = std::make_unique<Interpreter::AssignmentStatementNode>(
            "this", std::move(propertyPath), std::move(rhsNode), 
            this->current_filename_, thisTok.line_number, thisTok.column_number);
        
        // Add to operation container
        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::Assignment, "", std::move(assignNode) });
        
        return;
    }
    
    // Regular variable assignment (existing code)
    auto stmt = parseStatementNode();
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                          Operations::Operation{ Operations::Type::Assignment, "", std::move(stmt) });
}

// Parse an if-else conditional statement
void Parser::parseIfStatement() {
    // 'if'
    auto ifToken = expect(Lexer::Tokens::Type::KEYWORD_IF, "if");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse the condition expression without restricting literal types,
    // dynamic evaluation will enforce boolean type at runtime
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    // then branch
    std::vector<std::unique_ptr<Interpreter::StatementNode>> thenBranch;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        thenBranch.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    // else / else-if branch
    std::vector<std::unique_ptr<Interpreter::StatementNode>> elseBranch;
    if (match(Lexer::Tokens::Type::KEYWORD_ELSE, "else")) {
        // else-if: nested conditional
        if (currentToken().type == Lexer::Tokens::Type::KEYWORD_IF) {
            elseBranch.push_back(parseIfStatementNode());
        } else {
            expect(Lexer::Tokens::Type::PUNCTUATION, "{");
            while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
                elseBranch.push_back(parseStatementNode());
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        }
    }
    // build condition node
    auto condNode = buildExpressionFromParsed(condExpr);
    auto stmt     = std::make_unique<Interpreter::ConditionalStatementNode>(std::move(condNode), std::move(thenBranch),
                                                                            std::move(elseBranch), this->current_filename_,
                                                                            ifToken.line_number, ifToken.column_number);
    // add conditional operation
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::Conditional, "", std::move(stmt) });
}

// Parse an if-else conditional block and return a StatementNode (for nested blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseIfStatementNode() {
    auto ifToken = expect(Lexer::Tokens::Type::KEYWORD_IF, "if");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    std::vector<std::unique_ptr<Interpreter::StatementNode>> thenBranch;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        thenBranch.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    std::vector<std::unique_ptr<Interpreter::StatementNode>> elseBranch;
    if (match(Lexer::Tokens::Type::KEYWORD_ELSE, "else")) {
        // else-if: nested conditional
        if (currentToken().type == Lexer::Tokens::Type::KEYWORD_IF) {
            elseBranch.push_back(parseIfStatementNode());
        } else {
            expect(Lexer::Tokens::Type::PUNCTUATION, "{");
            while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
                elseBranch.push_back(parseStatementNode());
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        }
    }
    auto   condNode = buildExpressionFromParsed(condExpr);
    auto * node =
        new Interpreter::ConditionalStatementNode(std::move(condNode), std::move(thenBranch), std::move(elseBranch),
                                                  this->current_filename_, ifToken.line_number, ifToken.column_number);
    return std::unique_ptr<Interpreter::StatementNode>(node);
}

// Parse a for-in loop over object members and return a StatementNode (for nested blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseForStatementNode() {
    auto forToken = expect(Lexer::Tokens::Type::KEYWORD_FOR, "for");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse element type and variable name
    Symbols::Variables::Type elemType  = parseType();
    auto                     firstTok  = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
    std::string              firstName = firstTok.value;
    if (!firstName.empty() && firstName[0] == '$') {
        firstName = firstName.substr(1);
    }
    // C-style for loop: for (type $i = init; cond; incr) { ... }
    if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
        // Parse initialization expression
        auto initExpr = parseParsedExpression(elemType);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        // Parse condition expression
        auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        // Parse increment statement
        std::unique_ptr<Interpreter::StatementNode> incrStmt;
        {
            auto incrTok = currentToken();
            if (incrTok.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                auto        identTok = consumeToken();
                std::string incrName = identTok.value;
                if (!incrName.empty() && incrName[0] == '$') {
                    incrName = incrName.substr(1);
                }
                if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "++")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "+", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "--")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "-", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else {
                    reportError("Expected '++' or '--' in for-loop increment");
                }
            } else {
                reportError("Expected variable name in for-loop increment");
            }
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");
        expect(Lexer::Tokens::Type::PUNCTUATION, "{");
        // Parse loop body
        std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
        while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
            body.push_back(parseStatementNode());
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        // Build nodes for C-style for
        auto initExprNode = buildExpressionFromParsed(initExpr);
        auto initStmt     = std::make_unique<Interpreter::DeclareVariableStatementNode>(
            firstName, Symbols::SymbolContainer::instance()->currentScopeName(), elemType, std::move(initExprNode),
            this->current_filename_, firstTok.line_number, firstTok.column_number);
        auto   condExprNode = buildExpressionFromParsed(condExpr);
        auto * cnode        = new Interpreter::CStyleForStatementNode(
            std::move(initStmt), std::move(condExprNode), std::move(incrStmt), std::move(body), this->current_filename_,
            forToken.line_number, forToken.column_number);
        return std::unique_ptr<Interpreter::StatementNode>(cnode);
    }
    // Determine loop form: key,value or simple element loop
    std::string              keyName;
    std::string              valName;
    Symbols::Variables::Type keyType;
    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
        // Key, value syntax
        keyType = elemType;
        keyName = firstName;
        // Expect 'auto' for value variable
        if (!(currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "auto")) {
            reportError("Expected 'auto' in for-in loop");
        }
        consumeToken();
        auto valTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        valName     = valTok.value;
        if (!valName.empty() && valName[0] == '$') {
            valName = valName.substr(1);
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ":");
    } else if (match(Lexer::Tokens::Type::PUNCTUATION, ":")) {
        // Simple element loop
        keyType = elemType;
        keyName = firstName;
        valName = firstName;
    } else {
        reportError("Expected ',' or ':' in for-in loop");
    }
    auto iterableExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        body.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    auto   iterableExprNode = buildExpressionFromParsed(iterableExpr);
    auto * node =
        new Interpreter::ForStatementNode(keyType, keyName, valName, std::move(iterableExprNode), std::move(body),
                                          this->current_filename_, forToken.line_number, forToken.column_number);
    return std::unique_ptr<Interpreter::StatementNode>(node);
}

// Parse a while-in loop over object members and return a StatementNode (for nested blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseWhileStatementNode() {
    auto whileToken = expect(Lexer::Tokens::Type::KEYWORD_WHILE, "while");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        body.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    auto   condExprNode = buildExpressionFromParsed(condExpr);
    auto * node = new Interpreter::WhileStatementNode(std::move(condExprNode), std::move(body), this->current_filename_,
                                                      whileToken.line_number, whileToken.column_number);
    return std::unique_ptr<Interpreter::StatementNode>(node);
}

// Parse a single statement and return its StatementNode (for use in blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseStatementNode() {
    // Check if this is a special 'this' reference
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && 
        currentToken().value == "this" &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "->") {
        // Since we've already added code to handle 'this' in parseStatement(),
        // simply call the assignment statement parser which will handle it correctly
        parseAssignmentStatement();
        return nullptr;  // parseAssignmentStatement will create the operation
    }
    
    // Continue with normal statement parsing
    // Handle nested if statements in blocks
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_IF) {
        return parseIfStatementNode();
    }
    // Handle nested for loops in blocks
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_FOR) {
        return parseForStatementNode();
    }
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_WHILE) {
        return parseWhileStatementNode();
    }
    // Return statement
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        auto                tok  = expect(Lexer::Tokens::Type::KEYWORD_RETURN);
        ParsedExpressionPtr expr = nullptr;
        if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ";")) {
            expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = expr ? buildExpressionFromParsed(expr) : nullptr;
        return std::make_unique<Interpreter::ReturnStatementNode>(std::move(exprNode), this->current_filename_,
                                                                  tok.line_number, tok.column_number);
    }
    // Prefix increment/decrement (e.g., ++$u or --$u)
    if (currentToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT &&
        peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        auto opTok = expect(Lexer::Tokens::Type::OPERATOR_INCREMENT);
        auto idTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        std::string baseName = idTok.value;
        if (!baseName.empty() && baseName[0] == '$') {
            baseName = baseName.substr(1);
        }
        auto        lhs   = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
        auto        rhs   = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
        std::string binOp = (opTok.value == "++") ? "+" : "-";
        auto        bin   = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
        return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                      std::move(bin), this->current_filename_,
                                                                      idTok.line_number, idTok.column_number);
    }
    // Assignment statement: variable or object member assignment
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        // Postfix increment/decrement (e.g., $u++ or $u--)
        if (peekToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {
            auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string baseName = idTok.value;
            if (!baseName.empty() && baseName[0] == '$') {
                baseName = baseName.substr(1);
            }
            auto opTok = expect(Lexer::Tokens::Type::OPERATOR_INCREMENT);
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            std::unique_ptr<Interpreter::ExpressionNode> lhs =
                std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
            std::unique_ptr<Interpreter::ExpressionNode> rhs =
                std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
            std::string                                  binOp = (opTok.value == "++") ? "+" : "-";
            std::unique_ptr<Interpreter::ExpressionNode> bin =
                std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
            return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                          std::move(bin), this->current_filename_,
                                                                          idTok.line_number, idTok.column_number);
        }
        // Lookahead to detect '=' after optional '->' chains
        size_t offset = 1;
        // Skip member access sequence
        while (peekToken(offset).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(offset).value == "->") {
            offset += 2;  // skip '->' and following identifier
        }
        const auto & look = peekToken(offset);
        // Handle simple and compound assignment operators (e.g., =, +=, -=, etc.)
        if (look.type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT) {
            // Detect if base variable is 'this' (special case)
            if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "this") {
                // Consume 'this' keyword
                auto idTok = consumeToken();
                std::string baseName = "this";
                
                // Collect member path keys
                std::vector<std::string> propertyPath;
                while (match(Lexer::Tokens::Type::PUNCTUATION, "->")) {
                    // Next token must be identifier or variable identifier
                    Lexer::Tokens::Token propTok;
                    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                        propTok = consumeToken();
                    } else {
                        reportError("Expected property name after '->'");
                    }
                    std::string propName = propTok.value;
                    if (!propName.empty() && propName[0] == '$') {
                        propName = propName.substr(1);
                    }
                    propertyPath.push_back(propName);
                }
                
                // Consume assignment operator
                auto opTok = expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);
                
                // Parse RHS expression
                auto rhsExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                expect(Lexer::Tokens::Type::PUNCTUATION, ";");
                
                // Build RHS node
                auto rhsNode = buildExpressionFromParsed(rhsExpr);
                
                // Compound assignment: a OP= b -> a = a OP b
                if (opTok.value != "=") {
                    // Build LHS expression for current value
                    std::unique_ptr<Interpreter::ExpressionNode> lhsNode =
                        std::make_unique<Interpreter::IdentifierExpressionNode>("this");
                    
                    for (const auto & prop : propertyPath) {
                        lhsNode = std::make_unique<Interpreter::MemberExpressionNode>(
                            std::move(lhsNode), prop, this->current_filename_, opTok.line_number, opTok.column_number);
                    }
                    
                    // Extract binary operator ("+=" -> "+")
                    std::string binOp = opTok.value.substr(0, opTok.value.size() - 1);
                    
                    // Build binary expression lhs OP rhs
                    rhsNode =
                        std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhsNode), binOp, std::move(rhsNode));
                }
                
                return std::make_unique<Interpreter::AssignmentStatementNode>(
                    baseName, std::move(propertyPath), std::move(rhsNode), this->current_filename_,
                    idTok.line_number, idTok.column_number);
            }
            
            // Normal variable case (existing code)
            // Consume base variable
            auto idTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string baseName = idTok.value;
            if (!baseName.empty() && baseName[0] == '$') {
                baseName = baseName.substr(1);
            }
            
            // Special case for 'this' variable - 'this' is already registered in the method scope
            bool isThisVar = (baseName == "this");
            
            // Collect member path keys
            std::vector<std::string> propertyPath;
            while (match(Lexer::Tokens::Type::PUNCTUATION, "->")) {
                // Next token must be identifier or variable identifier
                Lexer::Tokens::Token propTok;
                if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                    currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                    propTok = consumeToken();
                } else {
                    reportError("Expected property name after '->'");
                }
                std::string propName = propTok.value;
                if (!propName.empty() && propName[0] == '$') {
                    propName = propName.substr(1);
                }
                propertyPath.push_back(propName);
            }
            // Consume assignment operator ("=" or compound like "+=")
            auto opTok   = expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);
            // Parse RHS expression
            auto rhsExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            // Build RHS node
            auto rhsNode = buildExpressionFromParsed(rhsExpr);
            // Compound assignment: a OP= b -> a = a OP b
            if (opTok.value != "=") {
                // Build LHS expression for current value
                std::unique_ptr<Interpreter::ExpressionNode> lhsNode;
                
                if (isThisVar) {
                    // 'this' is a special variable in method contexts
                    lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>("this");
                } else {
                    lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
                }
                
                for (const auto & prop : propertyPath) {
                    lhsNode = std::make_unique<Interpreter::MemberExpressionNode>(
                        std::move(lhsNode), prop, this->current_filename_, opTok.line_number, opTok.column_number);
                }
                // Extract binary operator ("+=" -> "+")
                std::string binOp = opTok.value.substr(0, opTok.value.size() - 1);
                // Build binary expression lhs OP rhs
                rhsNode =
                    std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhsNode), binOp, std::move(rhsNode));
            }
            return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::move(propertyPath),
                                                                          std::move(rhsNode), this->current_filename_,
                                                                          opTok.line_number, opTok.column_number);
        }
    }
    // Function call statement
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        auto        idTok    = expect(Lexer::Tokens::Type::IDENTIFIER);
        std::string funcName = idTok.value;
        expect(Lexer::Tokens::Type::PUNCTUATION, "(");
        std::vector<ParsedExpressionPtr> args;
        if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
            while (true) {
                args.push_back(parseParsedExpression(Symbols::Variables::Type::NULL_TYPE));
                if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                    continue;
                }
                break;
            }
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        std::vector<std::unique_ptr<Interpreter::ExpressionNode>> exprs;
        exprs.reserve(args.size());
        for (auto & p : args) {
            exprs.push_back(buildExpressionFromParsed(p));
        }
        return std::make_unique<Interpreter::CallStatementNode>(funcName, std::move(exprs), this->current_filename_,
                                                                idTok.line_number, idTok.column_number);
    }
    // Constant variable declaration in blocks
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_CONST) {
        // 'const'
        auto                     constTok = expect(Lexer::Tokens::Type::KEYWORD_CONST, "const");
        // Parse type
        Symbols::Variables::Type type     = parseType();
        // Variable name
        auto                     idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string              name     = idTok.value;
        if (!name.empty() && name[0] == '$') {
            name = name.substr(1);
        }
        // Assignment
        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        auto valExpr = parseParsedExpression(type);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = buildExpressionFromParsed(valExpr);
        return std::make_unique<Interpreter::DeclareVariableStatementNode>(
            name, Symbols::SymbolContainer::instance()->currentScopeName(), type, std::move(exprNode),
            this->current_filename_, idTok.line_number, idTok.column_number, /* isConst */ true);
    }
    // Variable declaration
    if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end()) {
        auto        type  = parseType();
        auto        idTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string name  = idTok.value;
        if (!name.empty() && name[0] == '$') {
            name = name.substr(1);
        }
        expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");
        auto valExpr = parseParsedExpression(type);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = buildExpressionFromParsed(valExpr);
        return std::make_unique<Interpreter::DeclareVariableStatementNode>(
            name, Symbols::SymbolContainer::instance()->currentScopeName(), type, std::move(exprNode),
            this->current_filename_, idTok.line_number, idTok.column_number);
    }
    // Expression statement: evaluate any other expression (e.g., method calls)
    {
        // Parse an expression and expect a terminating semicolon
        auto parsedExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        auto exprNode = buildExpressionFromParsed(parsedExpr);
        return std::make_unique<Interpreter::ExpressionStatementNode>(std::move(exprNode), this->current_filename_,
                                                                      parsedExpr->line, parsedExpr->column);
    }
    // Unexpected token in block
    reportError("Unexpected token in block");
    return nullptr;
}

// End of parseStatementNode
// Parse next definition or statement
// (Subsequent methods follow)

void Parser::parseFunctionDefinition() {
    expect(Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION);
    Lexer::Tokens::Token     id_token         = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string              func_name        = id_token.value;
    Symbols::Variables::Type func_return_type = Symbols::Variables::Type::NULL_TYPE;
    // note: '=' before parameter list is no longer required; function name is followed directly by '('
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");

    Symbols::FunctionParameterInfo param_infos;

    if (currentToken().type != Lexer::Tokens::Type::PUNCTUATION || currentToken().value != ")") {
        while (true) {
            // Parameter type
            Symbols::Variables::Type param_type = parseType();  // This consumes the type token

            // Parameter name ($variable)
            Lexer::Tokens::Token param_id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string          param_name     = param_id_token.value;
            if (!param_name.empty() && param_name[0] == '$') {  // remove '$'
                param_name = param_name.substr(1);
            }

            param_infos.push_back({ param_name, param_type });

            // Expecting comma or closing parenthesis?
            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                continue;
            }
            if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")") {
                break;  // end of list
            }
            reportError("Expected ',' or ')' in parameter list");
        }
    }
    // Now expect ')'
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    // check if we have a option return type: function name() type { ... }
    for (const auto & _type : Parser::variable_types) {
        if (match(_type.first)) {
            func_return_type = _type.second;
            break;
        }
    }

    // Consume '{' and record its token index for body extraction
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    size_t opening_brace_idx = current_token_index_ - 1;
    // Parse the function/method body using the brace index
    parseFunctionBody(opening_brace_idx, func_name, func_return_type, param_infos);
}

// Parse a top-level class definition: class Name { ... }
void Parser::parseClassDefinition() {
    // 'class'
    expect(Lexer::Tokens::Type::KEYWORD_CLASS);
    // Class name
    auto              nameToken = expect(Lexer::Tokens::Type::IDENTIFIER);
    const std::string className = nameToken.value;
    // Register class name early so parseType can recognize it as CLASS
    Symbols::ClassRegistry::instance().registerClass(className);
    // Enter class scope for methods and parsing
    const std::string fileNs  = Symbols::SymbolContainer::instance()->currentScopeName();
    // Use :: as namespace separator
    const std::string classNs = fileNs + "::" + className;
    // Create class scope (automatically enters it)
    Symbols::SymbolContainer::instance()->create(classNs);
    // Gather class members (registration happens at interpretation)
    std::vector<Symbols::ClassInfo::PropertyInfo> privateProps;
    std::vector<Symbols::ClassInfo::PropertyInfo> publicProps;
    std::vector<std::string>                      methodNames;

    enum AccessLevel : std::uint8_t { PRIVATE, PUBLIC } currentAccess = PRIVATE;

    // Opening brace
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    // Parse class body members
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        if (isAtEnd()) {
            reportError("Unterminated class definition");
        }
        auto tok = currentToken();
        // Access specifier
        // Access specifiers
        if (tok.type == Lexer::Tokens::Type::KEYWORD_PRIVATE) {
            // 'private:'
            consumeToken();
            expect(Lexer::Tokens::Type::PUNCTUATION, ":");
            currentAccess = PRIVATE;
            continue;
        }
        if (tok.type == Lexer::Tokens::Type::KEYWORD_PUBLIC) {
            // 'public:'
            consumeToken();
            expect(Lexer::Tokens::Type::PUNCTUATION, ":");
            currentAccess = PUBLIC;
            continue;
        }
        // Const property declaration
        if (tok.type == Lexer::Tokens::Type::KEYWORD_CONST) {
            consumeToken();
            // Type
            auto                 propType = parseType();
            // Property name
            Lexer::Tokens::Token idTok;
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                idTok = consumeToken();
            } else {
                reportError("Expected property name in class definition");
            }
            std::string propName = idTok.value;
            if (!propName.empty() && propName[0] == '$') {
                propName = propName.substr(1);
            }
            // Optional default value
            ParsedExpressionPtr defaultValue = nullptr;
            if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
                defaultValue = parseParsedExpression(propType);
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            Symbols::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue) };
            if (currentAccess == PRIVATE) {
                privateProps.push_back(std::move(info));
            } else {
                publicProps.push_back(std::move(info));
            }
            continue;
        }
        // Property declaration
        if (Parser::variable_types.find(tok.type) != Parser::variable_types.end() ||
            tok.type == Lexer::Tokens::Type::IDENTIFIER) {
            auto                 propType = parseType();
            // Property name
            Lexer::Tokens::Token idTok;
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                idTok = consumeToken();
            } else {
                reportError("Expected property name in class definition");
            }
            std::string propName = idTok.value;
            if (!propName.empty() && propName[0] == '$') {
                propName = propName.substr(1);
            }
            // Optional default value
            ParsedExpressionPtr defaultValue = nullptr;
            if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
                defaultValue = parseParsedExpression(propType);
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            Symbols::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue) };
            if (currentAccess == PRIVATE) {
                privateProps.push_back(std::move(info));
            } else {
                publicProps.push_back(std::move(info));
            }
            continue;
        }
        // Method declaration inside class
        if (tok.type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
            // Consume 'function'
            consumeToken();
            // Method name
            auto        nameId     = expect(Lexer::Tokens::Type::IDENTIFIER);
            std::string methodName = nameId.value;
            // Parsing method in current class scope; method bodies handled in parseFunctionBody
            // Parse parameters
            expect(Lexer::Tokens::Type::PUNCTUATION, "(");
            Symbols::FunctionParameterInfo params;
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                while (true) {
                    auto        paramType = parseType();
                    auto        paramTok  = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
                    std::string paramName = paramTok.value;
                    if (!paramName.empty() && paramName[0] == '$') {
                        paramName = paramName.substr(1);
                    }
                    params.push_back({ paramName, paramType });
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            // Optional return type
            Symbols::Variables::Type returnType = Symbols::Variables::Type::NULL_TYPE;
            for (const auto & vt : Parser::variable_types) {
                if (match(vt.first)) {
                    returnType = vt.second;
                    break;
                }
            }
            // Parse and record method body
            expect(Lexer::Tokens::Type::PUNCTUATION, "{");
            size_t opening_brace_idx = current_token_index_ - 1;
            parseFunctionBody(opening_brace_idx, methodName, returnType, params);
            // Track method name for class registry
            methodNames.push_back(methodName);
            continue;
        }
        // Unexpected token
        reportError("Unexpected token in class definition");
    }
    // Consume closing brace
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    // Exit class scope
    Symbols::SymbolContainer::instance()->enterPreviousScope();
    // Enqueue class definition for interpretation
    auto stmt = std::make_unique<Interpreter::ClassDefinitionStatementNode>(
        className, std::move(privateProps), std::move(publicProps), std::move(methodNames), this->current_filename_,
        nameToken.line_number, nameToken.column_number);
    Operations::Container::instance()->add(
        Symbols::SymbolContainer::instance()->currentScopeName(),
        Operations::Operation{ Operations::Type::Declaration, className, std::move(stmt) });
}

// Parse a top-level function call, e.g., foo(arg1, arg2);
void Parser::parseCallStatement() {
    // Function name
    auto        id_token  = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string func_name = id_token.value;
    // Opening parenthesis
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse comma-separated argument expressions
    std::vector<ParsedExpressionPtr> args;
    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
        while (true) {
            // Parse expression with no expected type
            auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            args.push_back(std::move(expr));
            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                continue;
            }
            break;
        }
    }
    // Closing parenthesis and semicolon
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
    // Record the function call operation
    Interpreter::OperationsFactory::callFunction(func_name, std::move(args),
                                                 Symbols::SymbolContainer::instance()->currentScopeName(),
                                                 this->current_filename_, id_token.line_number, id_token.column_number);
}

// Parse a return statement, e.g., return; or return expression;
void Parser::parseReturnStatement() {
    // Consume 'return' keyword
    auto                returnToken = expect(Lexer::Tokens::Type::KEYWORD_RETURN);
    // Parse optional expression
    ParsedExpressionPtr exprPtr;
    
    // Handle 'return this->$property' specially
    bool isThisAccess = false;
    bool hasExpr = false;
    
    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ";")) {
        hasExpr = true;
        
        // Special case for 'this' property access
        if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && 
            currentToken().value == "this" &&
            peekToken().type == Lexer::Tokens::Type::PUNCTUATION && 
            peekToken().value == "->") {
                
            isThisAccess = true;
            
            // Create a special expression for 'this' property access
            // Consume 'this'
            consumeToken();
            
            // Expect '->'
            expect(Lexer::Tokens::Type::PUNCTUATION, "->");
            
            // Get property name
            Lexer::Tokens::Token propTok;
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                propTok = consumeToken();
            } else {
                reportError("Expected property name after 'this->'");
            }
            
            std::string propName = propTok.value;
            if (!propName.empty() && propName[0] == '$') {
                propName = propName.substr(1);
            }
            
            // Create member expression for this->property
            auto thisExpr = std::make_unique<Interpreter::IdentifierExpressionNode>("this");
            auto memberExpr = std::make_unique<Interpreter::MemberExpressionNode>(
                std::move(thisExpr), propName, this->current_filename_,
                returnToken.line_number, returnToken.column_number);
                
            // Expect semicolon
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            
            // Create return statement
            auto returnStmt = std::make_unique<Interpreter::ReturnStatementNode>(
                std::move(memberExpr), this->current_filename_,
                returnToken.line_number, returnToken.column_number);
                
            // Record the return statement
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::Return, "", std::move(returnStmt) });
                
            return;
        }
        
        // Normal expression parsing
        exprPtr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    }
    
    // If this is a 'this' access, we've already handled it above
    if (!isThisAccess) {
        // Parse closing semicolon
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        
        // Build expression node
        std::unique_ptr<Interpreter::ExpressionNode> exprNode;
        if (hasExpr) {
            exprNode = buildExpressionFromParsed(exprPtr);
        }
        
        // Create return statement
        auto stmt = std::make_unique<Interpreter::ReturnStatementNode>(
            std::move(exprNode), this->current_filename_,
            returnToken.line_number, returnToken.column_number);
            
        // Record the return operation
        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::Return, "", std::move(stmt) });
    }
}

// Parse a for-in loop over object members
void Parser::parseForStatement() {
    // 'for'
    auto forToken = expect(Lexer::Tokens::Type::KEYWORD_FOR, "for");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse element type and variable name
    Symbols::Variables::Type elemType  = parseType();
    auto                     firstTok  = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
    std::string              firstName = firstTok.value;
    if (!firstName.empty() && firstName[0] == '$') {
        firstName = firstName.substr(1);
    }
    // C-style for loop: for (type $i = init; cond; incr) { ... }
    if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
        // Parse initialization expression
        auto initExpr = parseParsedExpression(elemType);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        // Parse condition expression
        auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        // Parse increment statement
        std::unique_ptr<Interpreter::StatementNode> incrStmt;
        {
            auto incrTok = currentToken();
            if (incrTok.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                auto        identTok = consumeToken();
                std::string incrName = identTok.value;
                if (!incrName.empty() && incrName[0] == '$') {
                    incrName = incrName.substr(1);
                }
                if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "++")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "+", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "--")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "-", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else {
                    reportError("Expected '++' or '--' in for-loop increment");
                }
            } else {
                reportError("Expected variable name in for-loop increment");
            }
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");
        expect(Lexer::Tokens::Type::PUNCTUATION, "{");
        // Parse loop body
        std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
        while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
            body.push_back(parseStatementNode());
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        // Build nodes for C-style for
        auto initExprNode = buildExpressionFromParsed(initExpr);
        auto initStmt     = std::make_unique<Interpreter::DeclareVariableStatementNode>(
            firstName, Symbols::SymbolContainer::instance()->currentScopeName(), elemType, std::move(initExprNode),
            this->current_filename_, firstTok.line_number, firstTok.column_number);
        auto condExprNode = buildExpressionFromParsed(condExpr);
        auto cnode        = std::make_unique<Interpreter::CStyleForStatementNode>(
            std::move(initStmt), std::move(condExprNode), std::move(incrStmt), std::move(body), this->current_filename_,
            forToken.line_number, forToken.column_number);
        Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                               Operations::Operation{ Operations::Type::Loop, "", std::move(cnode) });
        return;
    }
    // Determine loop form: key,value or simple element loop
    std::string              keyName;
    std::string              valName;
    Symbols::Variables::Type keyType;
    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
        // Key, value syntax
        keyType = elemType;
        keyName = firstName;
        // Expect 'auto' for value variable
        if (!(currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "auto")) {
            reportError("Expected 'auto' in for-in loop");
        }
        consumeToken();
        auto valTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        valName     = valTok.value;
        if (!valName.empty() && valName[0] == '$') {
            valName = valName.substr(1);
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ":");
    } else if (match(Lexer::Tokens::Type::PUNCTUATION, ":")) {
        // Simple element loop
        keyType = elemType;
        keyName = firstName;
        valName = firstName;
    } else {
        reportError("Expected ',' or ':' in for-in loop");
    }
    // Parse iterable expression
    auto iterableExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    // Parse loop body
    std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        body.push_back(parseStatementNode());
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    // Build expression and statement node
    auto iterableExprNode = buildExpressionFromParsed(iterableExpr);
    auto stmt = std::make_unique<Interpreter::ForStatementNode>(keyType, keyName, valName, std::move(iterableExprNode),
                                                                std::move(body), this->current_filename_,
                                                                forToken.line_number, forToken.column_number);
    // Record loop operation
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::Loop, "", std::move(stmt) });
}

void Parser::parseWhileStatement() {
    auto stmt = this->parseWhileStatementNode();

    // Create while loop operation
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::While, "", std::move(stmt) });
}

// Continue with numeric literal parsing
//
Symbols::Value Parser::parseNumericLiteral(const std::string & value, bool is_negative, Symbols::Variables::Type type) {
    try {
        switch (type) {
            case Symbols::Variables::Type::INTEGER:
                {
                    if (value.find('.') != std::string::npos) {
                        throw std::invalid_argument("Floating point value in integer context: " + value);
                    }
                    int v = std::stoi(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::DOUBLE:
                {
                    double v = std::stod(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::FLOAT:
                {
                    float v = std::stof(value);
                    return Symbols::Value(is_negative ? -v : v);
                }
            default:
                throw std::invalid_argument("Unsupported numeric type");
        }
    } catch (const std::invalid_argument & e) {
        reportError("Invalid numeric literal: " + value + " (" + e.what() + ")");
    } catch (const std::out_of_range & e) {
        reportError("Numeric literal out of range: " + value + " (" + e.what() + ")");
    }

    return Symbols::Value();  // unreachable
}

void Parser::parseFunctionBody(size_t opening_brace_idx, const std::string & function_name,
                               Symbols::Variables::Type return_type, const Symbols::FunctionParameterInfo & params) {
    // Find the matching closing brace for the function body
    size_t braceDepth  = 0;
    size_t closing_idx = opening_brace_idx;
    for (size_t i = opening_brace_idx + 1; i < tokens_.size(); ++i) {
        const auto & tok = tokens_[i];
        if (tok.type == Lexer::Tokens::Type::PUNCTUATION) {
            if (tok.value == "{") {
                ++braceDepth;
            } else if (tok.value == "}") {
                if (braceDepth == 0) {
                    closing_idx = i;
                    break;
                }
                --braceDepth;
            }
        }
    }
    if (closing_idx == opening_brace_idx) {
        reportError("Unmatched braces in function body");
    }
    // Extract tokens for the function body
    std::vector<Lexer::Tokens::Token> filtered_tokens(tokens_.begin() + opening_brace_idx + 1,
                                                      tokens_.begin() + closing_idx);
    // Extract the raw text for the body
    const auto &                      openTok      = tokens_[opening_brace_idx];
    const auto &                      closeTok     = tokens_[closing_idx];
    size_t                            start_pos    = openTok.end_pos;
    size_t                            len          = closeTok.start_pos - start_pos;
    std::string_view                  input_string = input_str_view_.substr(start_pos, len);

    // Advance parser index to closing '}' and consume it
    current_token_index_ = closing_idx;
    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    // Enter new namespace for the function body
    // Enter new namespace for the function body (use :: separator)
    const std::string newns = Symbols::SymbolContainer::instance()->currentScopeName() + "::" + function_name;
    Symbols::SymbolContainer::instance()->create(newns);
    // Parse function body in its own parser instance
    Parser innerParser;
    innerParser.parseScript(filtered_tokens, input_string, this->current_filename_);
    // Exit the function scope
    Symbols::SymbolContainer::instance()->enterPreviousScope();
    // Define the function symbol now that its body is recorded
    Interpreter::OperationsFactory::defineFunction(function_name, params, return_type,
                                                   Symbols::SymbolContainer::instance()->currentScopeName(),
                                                   this->current_filename_, openTok.line_number, openTok.column_number);
}

ParsedExpressionPtr Parser::parseParsedExpression(const Symbols::Variables::Type & expected_var_type) {
    std::stack<std::string>          operator_stack;
    std::vector<ParsedExpressionPtr> output_queue;
    // Reserve output queue to reduce reallocations
    if (tokens_.size() > current_token_index_) {
        output_queue.reserve(tokens_.size() - current_token_index_);
    }

    bool expect_unary = true;
    // Track if at start of expression (to distinguish array literal vs indexing)
    bool atStart      = true;

    while (true) {
        auto token = currentToken();
        // Handle 'new' keyword for object instantiation: new ClassName(arg1, arg2, ...)
        if (token.type == Lexer::Tokens::Type::KEYWORD_NEW) {
            // Consume 'new'
            auto newTok = consumeToken();
            // Next should be class name identifier
            if (currentToken().type != Lexer::Tokens::Type::IDENTIFIER) {
                reportError("Expected class name after 'new'");
            }
            auto        nameTok   = consumeToken();
            std::string className = nameTok.value;
            // Expect '('
            expect(Lexer::Tokens::Type::PUNCTUATION, "(");
            // Parse constructor arguments
            std::vector<ParsedExpressionPtr> args;
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                while (true) {
                    auto argExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                    args.push_back(std::move(argExpr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            // Closing ')'
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            // Create new expression
            output_queue.push_back(ParsedExpression::makeNew(className, std::move(args)));
            expect_unary = false;
            atStart      = false;
            continue;
        }
        // Array literal (at start) or dynamic indexing (postfix)
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "[") {
            if (atStart) {
                // Parse array literal as object with numeric keys
                consumeToken();  // consume '['
                std::vector<std::pair<std::string, ParsedExpressionPtr>> members;
                size_t                                                   idx = 0;
                // Elements until ']'
                if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "]")) {
                    while (true) {
                        auto elem = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                        members.emplace_back(std::to_string(idx++), std::move(elem));
                        if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                            continue;
                        }
                        break;
                    }
                }
                expect(Lexer::Tokens::Type::PUNCTUATION, "]");
                // Build as object literal
                output_queue.push_back(ParsedExpression::makeObject(std::move(members)));
                expect_unary = false;
                atStart      = false;
                continue;
            }  // Parse dynamic array/object indexing: lhs[index]
            consumeToken();  // consume '['
            auto indexExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            expect(Lexer::Tokens::Type::PUNCTUATION, "]");
            if (output_queue.empty()) {
                reportError("Missing array/object for indexing");
            }
            auto lhsExpr = std::move(output_queue.back());
            output_queue.pop_back();
            auto accessExpr = ParsedExpression::makeBinary("[]", std::move(lhsExpr), std::move(indexExpr));
            output_queue.push_back(std::move(accessExpr));
            expect_unary = false;
            atStart      = false;
            continue;
        }
        // Object literal: { key: value, ... }
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "{") {
            // Consume '{'
            consumeToken();
            std::vector<std::pair<std::string, ParsedExpressionPtr>> members;
            // Parse members until '}'
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
                while (true) {
                    // Optional type tag before key
                    Symbols::Variables::Type memberType = Symbols::Variables::Type::UNDEFINED_TYPE;
                    if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end()) {
                        memberType = parseType();
                    }
                    // Key must be an identifier or variable identifier
                    if (currentToken().type != Lexer::Tokens::Type::IDENTIFIER &&
                        currentToken().type != Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                        reportError("Expected identifier for object key");
                    }
                    std::string key = currentToken().value;
                    // Strip '$' if present
                    if (!key.empty() && key[0] == '$') {
                        key = key.substr(1);
                    }
                    consumeToken();
                    // Expect ':' delimiter
                    expect(Lexer::Tokens::Type::PUNCTUATION, ":");
                    // Parse value expression (pass tag type if provided)
                    Symbols::Variables::Type expectType = (memberType == Symbols::Variables::Type::UNDEFINED_TYPE) ?
                                                              Symbols::Variables::Type::NULL_TYPE :
                                                              memberType;
                    auto                     valueExpr  = parseParsedExpression(expectType);
                    members.emplace_back(key, std::move(valueExpr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            // Expect closing '}'
            expect(Lexer::Tokens::Type::PUNCTUATION, "}");
            // Create object literal parsed expression
            output_queue.push_back(ParsedExpression::makeObject(std::move(members)));
            expect_unary = false;
            continue;
        }

        // Member access: '->'
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "->") {
            std::string op(token.lexeme);
            // Shunting-yard: handle operator precedence
            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                if ((Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) <= Lexer::getPrecedence(top)) ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) < Lexer::getPrecedence(top))) {
                    operator_stack.pop();
                    // Binary operator: pop two operands
                    if (output_queue.size() < 2) {
                        Parser::reportError("Malformed expression", token);
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    auto lhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
                } else {
                    break;
                }
            }
            operator_stack.push(op);
            consumeToken();
            
            // Check if the next token is a variable identifier with a $ prefix (e.g., this->$property)
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                Lexer::Tokens::Token propToken = consumeToken();
                std::string propName = propToken.value;
                // Remove $ prefix from property name
                if (!propName.empty() && propName[0] == '$') {
                    propName = propName.substr(1);
                }
                // Create an identifier for the property without $ and push to queue
                output_queue.push_back(ParsedExpression::makeVariable(propName));
                expect_unary = false;
                atStart = false;
                continue;
            }
            
            expect_unary = true;
        }
        // Grouping parentheses closing
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == ")") {
            // Only handle grouping parentheses if a matching "(" exists on the operator stack
            std::stack<std::string> temp_stack = operator_stack;
            bool                    has_paren  = false;
            while (!temp_stack.empty()) {
                if (temp_stack.top() == "(") {
                    has_paren = true;
                    break;
                }
                temp_stack.pop();
            }
            if (!has_paren) {
                // End of this expression context; do not consume call-closing parenthesis here
                break;
            }
            // Consume the grouping closing parenthesis
            consumeToken();
            // Unwind operators until the matching "(" is found
            while (!operator_stack.empty() && operator_stack.top() != "(") {
                std::string op = operator_stack.top();
                operator_stack.pop();

                if (op == "u-" || op == "u+") {
                    if (output_queue.empty()) {
                        reportError("Missing operand for unary operator");
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs)));
                } else {
                    if (output_queue.size() < 2) {
                        reportError("Malformed expression");
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    auto lhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
                }
            }
            if (operator_stack.empty() || operator_stack.top() != "(") {
                Parser::reportError("Mismatched parentheses", token);
            }
            // Pop the matching "("
            operator_stack.pop();
            expect_unary = false;
        }
        // Method or function call: expression followed by '('
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "(") {
            // If we have a preceding expression, treat as call: pop callee from output_queue
            if (!expect_unary && !output_queue.empty()) {
                // Consume '('
                consumeToken();
                // Parse arguments
                std::vector<ParsedExpressionPtr> args;
                if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                    while (true) {
                        auto arg = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                        args.push_back(std::move(arg));
                        if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                            continue;
                        }
                        break;
                    }
                }
                expect(Lexer::Tokens::Type::PUNCTUATION, ")");
                // Build call: callee could be variable or member expression
                auto callee = std::move(output_queue.back());
                output_queue.pop_back();
                ParsedExpressionPtr callExpr;
                if (callee->kind == ParsedExpression::Kind::Binary && callee->op == "->") {
                    // Method call: object->method(args)
                    callExpr =
                        ParsedExpression::makeMethodCall(std::move(callee->lhs), callee->rhs->name, std::move(args));
                } else if (callee->kind == ParsedExpression::Kind::Variable) {
                    // Function call
                    callExpr = ParsedExpression::makeCall(callee->name, std::move(args));
                } else {
                    reportError("Invalid call target");
                }
                // Preserve source location
                callExpr->filename = callee->filename;
                callExpr->line     = callee->line;
                callExpr->column   = callee->column;
                output_queue.push_back(std::move(callExpr));
                expect_unary = false;
                atStart      = false;
                continue;
            }
            // Fallback grouping: treat '(' as usual
            operator_stack.push(token.value);
            consumeToken();
            expect_unary = true;
            continue;
        } else if (token.type == Lexer::Tokens::Type::IDENTIFIER &&
                   peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
            // Parse function call
            std::string func_name = token.value;
            consumeToken();  // consume function name
            consumeToken();  // consume '('
            std::vector<ParsedExpressionPtr> call_args;
            // Parse arguments if any
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                while (true) {
                    auto arg_expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                    call_args.push_back(std::move(arg_expr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            // Create call expression node with source location
            {
                auto pe      = ParsedExpression::makeCall(func_name, std::move(call_args));
                pe->filename = this->current_filename_;
                pe->line     = token.line_number;
                pe->column   = token.column_number;
                output_queue.push_back(std::move(pe));
            }
            expect_unary = false;
        } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC ||
                   token.type == Lexer::Tokens::Type::OPERATOR_RELATIONAL ||
                   token.type == Lexer::Tokens::Type::OPERATOR_LOGICAL) {
            std::string op = std::string(token.lexeme);

            if (expect_unary && Lexer::isUnaryOperator(op)) {
                op = "u" + op;  // e.g. u-, u+ or u!
            }

            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                if ((Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) <= Lexer::getPrecedence(top)) ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) < Lexer::getPrecedence(top))) {
                    operator_stack.pop();

                    if (top == "u-" || top == "u+") {
                        if (output_queue.empty()) {
                            Parser::reportError("Missing operand for unary operator", token);
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(Lexer::applyOperator(top, std::move(rhs)));
                    } else {
                        if (output_queue.size() < 2) {
                            Parser::reportError("Malformed expression", token);
                        }
                        auto rhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        auto lhs = std::move(output_queue.back());
                        output_queue.pop_back();
                        output_queue.push_back(Lexer::applyOperator(top, std::move(rhs), std::move(lhs)));
                    }
                } else {
                    break;
                }
            }

            operator_stack.push(op);
            consumeToken();
            expect_unary = true;
        } else if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                   token.type == Lexer::Tokens::Type::KEYWORD ||
                   token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                   token.type == Lexer::Tokens::Type::IDENTIFIER) {
          
            // Special case for 'this->$property' syntax
            if (token.type == Lexer::Tokens::Type::IDENTIFIER && 
                token.value == "this" && 
                peekToken().type == Lexer::Tokens::Type::PUNCTUATION && 
                peekToken().value == "->") {
                
                // Use helper method to parse 'this->$property' access
                output_queue.push_back(parseThisPropertyAccess());
            }
            else if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
                // Special case for 'this' keyword in method contexts
                if (token.value == "this") {
                    output_queue.push_back(ParsedExpression::makeVariable("this"));
                } else {
                    // Treat other bare identifiers as variable references for member access
                    output_queue.push_back(ParsedExpression::makeVariable(token.value));
                }
                consumeToken();
            } else {
                if (Lexer::pushOperand(token, expected_var_type, output_queue) == false) {
                    Parser::reportError("Invalid type", token, "literal or variable");
                }
                consumeToken();
            }
            
            expect_unary = false;
            atStart = false;
        } else {
            break;
        }
    }

    // Empty the operator stack
    while (!operator_stack.empty()) {
        std::string op = operator_stack.top();
        operator_stack.pop();

        if (op == "(" || op == ")") {
            Parser::reportError("Mismatched parentheses", tokens_[current_token_index_]);
        }

        // Handle unary operators (plus, minus, logical NOT)
        if (op == "u-" || op == "u+" || op == "u!") {
            if (output_queue.empty()) {
                Parser::reportError("Missing operand for unary operator", tokens_[current_token_index_]);
            }
            auto rhs = std::move(output_queue.back());
            output_queue.pop_back();
            output_queue.push_back(Lexer::applyOperator(op, std::move(rhs)));
        } else {
            // Binary operators
            if (output_queue.size() < 2) {
                Parser::reportError("Malformed expression", tokens_[current_token_index_]);
            }
            auto rhs = std::move(output_queue.back());
            output_queue.pop_back();
            auto lhs = std::move(output_queue.back());
            output_queue.pop_back();
            output_queue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
        }
    }

    if (output_queue.size() != 1) {
        reportError("Expression could not be parsed cleanly");
    }

    return std::move(output_queue.back());
}

void Parser::parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                         const std::string & filename) {
    ::Parser::Parser::Exception::current_filename_ = filename;
    tokens_                                        = tokens;
    input_str_view_                                = input_string;
    current_token_index_                           = 0;
    current_filename_                              = filename;

    while (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        parseStatement();
    }
    if (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        reportError("Unexpected tokens after program end");
    }
}

void Parser::parseStatement() {
    const auto & token_type = currentToken().type;
    // if-else conditional
    if (token_type == Lexer::Tokens::Type::KEYWORD_IF) {
        parseIfStatement();
        return;
    }

    if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
        parseFunctionDefinition();
        return;
    }
    // Return statement
    if (token_type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        parseReturnStatement();
        return;
    }
    // For-in loop over object members
    if (token_type == Lexer::Tokens::Type::KEYWORD_FOR) {
        parseForStatement();
        return;
    }
    // while loop
    if (token_type == Lexer::Tokens::Type::KEYWORD_WHILE) {
        parseWhileStatement();
        return;
    }

    // Class definition
    if (token_type == Lexer::Tokens::Type::KEYWORD_CLASS) {
        parseClassDefinition();
        return;
    }
    // Constant variable definition
    if (token_type == Lexer::Tokens::Type::KEYWORD_CONST) {
        parseConstVariableDefinition();
        return;
    }
    // Variable definition if leading token matches a type keyword
    if (Parser::variable_types.find(token_type) != Parser::variable_types.end()) {
        parseVariableDefinition();
        return;
    }
    // Variable definition for user-defined class types (e.g., ClassName $var)
    if (token_type == Lexer::Tokens::Type::IDENTIFIER && peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        parseVariableDefinition();
        return;
    }
    // Function call if identifier followed by '('
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        parseCallStatement();
        return;
    }
    // Assignment statement at top-level or method call on an object
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "this")) {
        
        // Special handling for 'this' keyword  
        bool isThisKeyword = (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && 
                              currentToken().value == "this");
        
        // Detect simple method call pattern: $obj->method(...) or this->method(...)
        if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "->" &&
            peekToken(2).type == Lexer::Tokens::Type::IDENTIFIER &&
            peekToken(3).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(3).value == "(") {
            
            // Parse base variable (could be $variable or this)
            Lexer::Tokens::Token varTok;
            std::string varName;
            
            if (isThisKeyword) {
                varTok = consumeToken();  // Consume 'this'
                varName = "this";
            } else {
                varTok = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
                varName = varTok.value;
                if (!varName.empty() && varName[0] == '$') {
                    varName = varName.substr(1);
                }
            }
            
            // Build object expression node
            std::unique_ptr<Interpreter::ExpressionNode> objectExpr =
                std::make_unique<Interpreter::IdentifierExpressionNode>(varName);
            // Consume '->'
            expect(Lexer::Tokens::Type::PUNCTUATION, "->");
            // Method name
            auto        methodTok  = expect(Lexer::Tokens::Type::IDENTIFIER);
            std::string methodName = methodTok.value;
            // Parse arguments
            expect(Lexer::Tokens::Type::PUNCTUATION, "(");
            std::vector<std::unique_ptr<Interpreter::ExpressionNode>> args;
            if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
                while (true) {
                    auto argPexpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                    args.push_back(buildExpressionFromParsed(argPexpr));
                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");
            // Create method call expression node
            auto methodCall = std::make_unique<Interpreter::MethodCallExpressionNode>(
                std::move(objectExpr), methodName, std::move(args), this->current_filename_, varTok.line_number,
                varTok.column_number);
            // Wrap into expression statement and record operation
            auto exprStmt = std::make_unique<Interpreter::ExpressionStatementNode>(
                std::move(methodCall), this->current_filename_, varTok.line_number, varTok.column_number);
            auto ns = Symbols::SymbolContainer::instance()->currentScopeName();
            Operations::Container::instance()->add(
                ns, Operations::Operation{ Operations::Type::Expression, std::string(), std::move(exprStmt) });
            return;
        }
        
        // Postfix increment/decrement (e.g., $u++ or $u--)
        if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER && 
            peekToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {
            parseAssignmentStatement();
            return;
        }
        
        // Handle assignments like $var = x, this->$prop = x, or $obj->$prop = x
        // Fallback to assignment for variable-based LHS or 'this'
        size_t offset = 1;
        // Skip property access chain to find assignment
        while (peekToken(offset).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(offset).value == "->") {
            offset += 2;
        }
        const auto & look = peekToken(offset);
        if (look.type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT) {
            parseAssignmentStatement();
            return;
        }
    }

    reportError("Unexpected token at beginning of statement");
}

Symbols::Variables::Type Parser::parseType() {
    const auto & token = currentToken();
    // Direct lookup for type keyword
    auto         it    = Parser::variable_types.find(token.type);
    if (it != Parser::variable_types.end()) {
        // Base type
        consumeToken();
        // Array type syntax: baseType[] -> treat as OBJECT/array map
        if (match(Lexer::Tokens::Type::PUNCTUATION, "[") && match(Lexer::Tokens::Type::PUNCTUATION, "]")) {
            return Symbols::Variables::Type::OBJECT;
        }
        return it->second;
    }
    // User-defined class types: if identifier names a registered class, return CLASS; otherwise OBJECT
    if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
        // Capture the identifier value as potential class name
        const std::string typeName = token.value;
        // Consume the identifier token
        consumeToken();
        // If this name is a defined class, treat as CLASS
        if (Symbols::ClassRegistry::instance().hasClass(typeName)) {
            return Symbols::Variables::Type::CLASS;
        }
        // Otherwise treat as generic object type
        return Symbols::Variables::Type::OBJECT;
    }
    reportError("Expected type keyword (string, int, double, float or class name)");
}

Symbols::Value Parser::parseValue(Symbols::Variables::Type expected_var_type) {
    Lexer::Tokens::Token token       = currentToken();
    bool                 is_negative = false;

    // Handle unary sign
    if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && (token.lexeme == "-" || token.lexeme == "+") &&
        peekToken().type == Lexer::Tokens::Type::NUMBER) {
        is_negative = (token.lexeme == "-");
        token       = peekToken();
        consumeToken();  // consumed the sign
    }

    // STRING type
    if (expected_var_type == Symbols::Variables::Type::STRING) {
        if (token.type == Lexer::Tokens::Type::STRING_LITERAL) {
            consumeToken();
            return Symbols::Value(token.value);
        }
        reportError("Expected string literal value");
    }

    // BOOLEAN type
    if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
        if (token.type == Lexer::Tokens::Type::KEYWORD && (token.value == "true" || token.value == "false")) {
            consumeToken();
            return Symbols::Value(token.value == "true");
        }
        reportError("Expected boolean literal value (true or false)");
    }

    // NUMERIC types
    if (expected_var_type == Symbols::Variables::Type::INTEGER ||
        expected_var_type == Symbols::Variables::Type::DOUBLE || expected_var_type == Symbols::Variables::Type::FLOAT) {
        if (token.type == Lexer::Tokens::Type::NUMBER) {
            Symbols::Value val = parseNumericLiteral(token.value, is_negative, expected_var_type);
            consumeToken();
            return val;
        }

        reportError("Expected numeric literal value");
    }

    reportError("Unsupported variable type encountered during value parsing");
    return Symbols::Value();  // compiler happy
}

bool Parser::isAtEnd() const {
    // We're at the end if the index equals the number of tokens,
    // or if only the EOF token remains (as the last element)
    return current_token_index_ >= tokens_.size() ||
           (current_token_index_ == tokens_.size() - 1 && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE);
}

Lexer::Tokens::Token Parser::expect(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
    if (isAtEnd()) {
        reportError("Unexpected end of file, expected token: " + Lexer::Tokens::TypeToString(expected_type) +
                    " with value '" + expected_value + "'");
    }
    const auto & token = currentToken();
    if (token.type == expected_type && token.value == expected_value) {
        return consumeToken();
    }
    reportError("Expected token " + Lexer::Tokens::TypeToString(expected_type) + " with value '" + expected_value +
                "'");
    return token;  // reportError throws
}

Lexer::Tokens::Token Parser::expect(Lexer::Tokens::Type expected_type) {
    if (isAtEnd()) {
        reportError("Unexpected end of file, expected token type: " + Lexer::Tokens::TypeToString(expected_type));
    }
    const auto & token = currentToken();
    if (token.type == expected_type) {
        return consumeToken();
    }
    reportError("Expected token type " + Lexer::Tokens::TypeToString(expected_type));
    // reportError throws; this return is never reached, but may satisfy the compiler
    return token;  // or let reportError throw
}

bool Parser::match(Lexer::Tokens::Type expected_type, const std::string & expected_value) {
    if (isAtEnd()) {
        return false;
    }
    const auto & token = currentToken();
    if (token.type == expected_type && token.value == expected_value) {
        consumeToken();
        return true;
    }
    return false;
}

bool Parser::match(Lexer::Tokens::Type expected_type) {
    if (isAtEnd()) {
        return false;
    }
    if (currentToken().type == expected_type) {
        consumeToken();
        return true;
    }
    return false;
}

Lexer::Tokens::Token Parser::consumeToken() {
    if (isAtEnd()) {
        throw std::runtime_error("Cannot consume token at end of stream.");
    }
    return tokens_[current_token_index_++];
}

const Lexer::Tokens::Token & Parser::peekToken(size_t offset) const {
    if (current_token_index_ + offset >= tokens_.size()) {
        // If at or beyond EOF, return the last token (should be EOF)
        if (!tokens_.empty()) {
            return tokens_.back();
        }
        throw std::runtime_error("Cannot peek beyond end of token stream.");
    }
    return tokens_[current_token_index_ + offset];
}

const Lexer::Tokens::Token & Parser::currentToken() const {
    if (isAtEnd()) {
        // Technically we should never reach this if parseScript's loop is correct
        // But it's useful as a safety check
        if (!tokens_.empty() && tokens_.back().type == Lexer::Tokens::Type::END_OF_FILE) {
            return tokens_.back();  // return the EOF token
        }
        throw std::runtime_error("Unexpected end of token stream reached.");
    }
    return tokens_[current_token_index_];
}

// Add this helper method to parse a 'this->$property' access as a special case
ParsedExpressionPtr Parser::parseThisPropertyAccess() {
    // Consume 'this' token
    auto thisTok = consumeToken();
    
    // Expect '->' token
    expect(Lexer::Tokens::Type::PUNCTUATION, "->");
    
    // Get property name (with or without $)
    Lexer::Tokens::Token propTok;
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        propTok = consumeToken();
    } else {
        reportError("Expected property name after 'this->'");
    }
    
    std::string propName = propTok.value;
    if (!propName.empty() && propName[0] == '$') {
        propName = propName.substr(1);
    }
    
    // Create a variable expression for 'this'
    auto thisExpr = ParsedExpression::makeVariable("this");
    
    // Create a member expression for the property access
    auto memberExpr = ParsedExpression::makeMember(std::move(thisExpr), propName);
    
    // Add source location for better error messages
    memberExpr->filename = this->current_filename_;
    memberExpr->line = thisTok.line_number;
    memberExpr->column = thisTok.column_number;
    
    return memberExpr;
}
}  // namespace Parser
