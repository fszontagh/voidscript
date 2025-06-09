#include "Parser/Parser.hpp"

#include <fstream>
#include <sstream>
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
#include "Interpreter/Nodes/Statement/EnumDeclarationNode.hpp" // Added for EnumDeclarationNode
#include "Interpreter/Nodes/Statement/BreakNode.hpp"         // Added for BreakNode
#include "Interpreter/Nodes/Statement/SwitchStatementNode.hpp" // Added for SwitchStatementNode
#include "Interpreter/OperationsFactory.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Operators.hpp"
#include "Parser.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "utils.h"

// Additional necessary includes, if needed
namespace Parser {

// Static filename for unified error reporting in Parser::Exception
std::string Parser::Parser::Exception::current_filename_;

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
    { "this",     Lexer::Tokens::Type::KEYWORD_THIS                 },
    { "true",     Lexer::Tokens::Type::KEYWORD                      },
    { "false",    Lexer::Tokens::Type::KEYWORD                      },
    { "include",  Lexer::Tokens::Type::KEYWORD_INCLUDE              },
    // variable types
    { "null",     Lexer::Tokens::Type::KEYWORD_NULL                 },
    { "int",      Lexer::Tokens::Type::KEYWORD_INT                  },
    { "double",   Lexer::Tokens::Type::KEYWORD_DOUBLE               },
    { "float",    Lexer::Tokens::Type::KEYWORD_FLOAT                },
    { "string",   Lexer::Tokens::Type::KEYWORD_STRING               },
    { "boolean",  Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "bool",     Lexer::Tokens::Type::KEYWORD_BOOLEAN              },
    { "object",   Lexer::Tokens::Type::KEYWORD_OBJECT               },
    { "include",  Lexer::Tokens::Type::KEYWORD_INCLUDE              },
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
    { Lexer::Tokens::Type::KEYWORD_ENUM,    Symbols::Variables::Type::INTEGER   } // Added for enum type
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
    std::string var_name = Parser::parseIdentifierName(id_token);
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
    std::string var_name = Parser::parseIdentifierName(id_token);
    // Corrected: ns should be the pure scope name, not combined with sub-namespace here.
    const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();

    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");

    auto expr = parseParsedExpression(var_type);
    Interpreter::OperationsFactory::defineVariableWithExpression(var_name, var_type, expr, ns, current_filename_,
                                                                 id_token.line_number, id_token.column_number);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");
}

// Parse a top-level assignment statement and record it
void Parser::parseAssignmentStatement() {
    // Use the node parsing logic and add the result to the container
    auto stmt = parseAssignmentStatementNode();
    if (stmt) {  // Ensure a valid node was returned
        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::Assignment, "", std::move(stmt) });
    }
}

// Parse an if-else conditional statement
void Parser::parseIfStatement() {
    auto stmt = parseIfStatementNode();
    // add conditional operation
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::Conditional, "", std::move(stmt) });
}

// Helper method to parse a statement body enclosed in { }
std::vector<std::unique_ptr<Interpreter::StatementNode>> Parser::parseStatementBody(const std::string & errorContext) {
    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    std::vector<std::unique_ptr<Interpreter::StatementNode>> body;
    while (!match(Lexer::Tokens::Type::PUNCTUATION, "}")) {
        if (isAtEnd()) {
            reportError("Unterminated block in " + errorContext);
        }
        auto stmt = parseStatementNode();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }
    return body;
}

// Parse an if-else conditional block and return a StatementNode (for nested blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseIfStatementNode() {
    auto ifToken = expect(Lexer::Tokens::Type::KEYWORD_IF, "if");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Dynamic evaluation will enforce boolean type at runtime
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    // then branch
    auto thenBranch = parseStatementBody("if statement");

    // else / else-if branch
    std::vector<std::unique_ptr<Interpreter::StatementNode>> elseBranch;
    if (match(Lexer::Tokens::Type::KEYWORD_ELSE, "else")) {
        // else-if: nested conditional
        if (currentToken().type == Lexer::Tokens::Type::KEYWORD_IF) {
            auto stmt = parseIfStatementNode();
            if (stmt) {
                elseBranch.push_back(std::move(stmt));
            }
        } else {
            elseBranch = parseStatementBody("else statement");
        }
    }

    // build condition node
    auto condNode = buildExpressionFromParsed(condExpr);
    return std::make_unique<Interpreter::ConditionalStatementNode>(std::move(condNode), std::move(thenBranch),
                                                                   std::move(elseBranch), this->current_filename_,
                                                                   ifToken.line_number, ifToken.column_number);
}

// Parse a for loop (C-style or for-in) and return a StatementNode
std::unique_ptr<Interpreter::StatementNode> Parser::parseForStatementNode() {
    auto forToken = expect(Lexer::Tokens::Type::KEYWORD_FOR, "for");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");

    const std::string currentScope = Symbols::SymbolContainer::instance()->currentScopeName();

    // Parse element type and variable name (common to both loop types)
    Symbols::Variables::Type elemType  = parseType();
    auto                     firstTok  = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
    std::string              firstName = parseIdentifierName(firstTok);  // Use helper

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
        // ... (rest of C-style increment parsing remains the same) ...
        {
            auto incrTok = currentToken();
            if (incrTok.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                auto        identTok = consumeToken();
                std::string incrName = parseIdentifierName(identTok);  // Use helper

                if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "++")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName, this->current_filename_, incrTok.line_number, incrTok.column_number);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "+", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else if (match(Lexer::Tokens::Type::OPERATOR_INCREMENT, "--")) {
                    auto lhs = std::make_unique<Interpreter::IdentifierExpressionNode>(incrName, this->current_filename_, incrTok.line_number, incrTok.column_number);
                    auto rhs = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
                    auto bin = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), "-", std::move(rhs));
                    incrStmt = std::make_unique<Interpreter::AssignmentStatementNode>(
                        incrName, std::vector<std::string>(), std::move(bin), this->current_filename_,
                        incrTok.line_number, incrTok.column_number);
                } else {
                    reportError("Expected '++' or '--' in for-loop increment", incrTok);
                }
            } else if (incrTok.type == Lexer::Tokens::Type::PUNCTUATION && incrTok.value == ")") {
                // Empty increment statement
                incrStmt = nullptr;
            } else {
                reportError("Expected variable name or ')' in for-loop increment", incrTok);
            }
        }
        expect(Lexer::Tokens::Type::PUNCTUATION, ")");

        // Parse loop body (within loopScope)
        auto body = parseStatementBody("C-style for loop");

        // Build nodes for C-style for
        auto initExprNode = buildExpressionFromParsed(initExpr);
        // Create initStmt node targeting loopScope
        auto initStmt     = std::make_unique<Interpreter::DeclareVariableStatementNode>(
            firstName, currentScope, elemType, std::move(initExprNode), this->current_filename_, firstTok.line_number,
            firstTok.column_number);
        // NO Operations::add here
        auto condExprNode = buildExpressionFromParsed(condExpr);

        // Create the CStyleForStatementNode
        auto forNode = std::make_unique<Interpreter::CStyleForStatementNode>(
            std::move(initStmt), std::move(condExprNode), std::move(incrStmt), std::move(body), this->current_filename_,
            forToken.line_number, forToken.column_number);

        // Exit the loop scope (for subsequent parsing)
        //Symbols::SymbolContainer::instance()->enterPreviousScope();

        return forNode;
    }

    // For-in loop variant (check starts after C-style check fails)
    std::string              keyName;
    std::string              valName;
    Symbols::Variables::Type keyType;  // Keep track of declared key type
    // Parse loop variable declarations
    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
        // Key, value syntax: for (keyType $key, auto $value : iterable)
        keyType                          = elemType;     // Type before comma is key type
        keyName                          = firstName;
        Symbols::Variables::Type valType = parseType();  // Get value type (might be auto/handled later)
        auto                     valTok  = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        valName                          = parseIdentifierName(valTok);
        expect(Lexer::Tokens::Type::PUNCTUATION, ":");
    } else if (match(Lexer::Tokens::Type::PUNCTUATION, ":")) {
        // Simple element loop: for (elemType $element : iterable)
        keyType = elemType;   // Declared type is for the element (value)
        keyName = firstName;  // Use first name for key var name
        valName = firstName;  // Use first name for value var name too
    } else {
        reportError("Expected ',' or ':' in for loop after variable declaration", firstTok);
        return nullptr;  // Should not be reached
    }

    auto iterableExpr =
        parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);  // Iterable can be any type (object/array)
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    // Parse body (within the loopScope created earlier)
    auto body             = parseStatementBody("for-in loop");
    auto iterableExprNode = buildExpressionFromParsed(iterableExpr);

    // Pass loopScope (created at function start) to the ForStatementNode constructor
    auto forNode = std::make_unique<Interpreter::ForStatementNode>(
        keyType, keyName, valName, std::move(iterableExprNode), std::move(body), currentScope,  // <-- Pass loopScope
        this->current_filename_, forToken.line_number, forToken.column_number);

    // Exit the loop scope (for subsequent parsing)
    Symbols::SymbolContainer::instance()->enterPreviousScope();

    return forNode;
}

// Parse a while loop over object members and return a StatementNode (for nested blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseWhileStatementNode() {
    auto whileToken = expect(Lexer::Tokens::Type::KEYWORD_WHILE, "while");
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    auto condExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    auto body         = parseStatementBody("while loop");
    auto condExprNode = buildExpressionFromParsed(condExpr);
    return std::make_unique<Interpreter::WhileStatementNode>(std::move(condExprNode), std::move(body),
                                                             this->current_filename_, whileToken.line_number,
                                                             whileToken.column_number);
}

// Parse a single statement and return its StatementNode (for use in blocks)
std::unique_ptr<Interpreter::StatementNode> Parser::parseStatementNode() {
    // Handle keywords that start statements
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_IF) {
        return parseIfStatementNode();
    }
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_FOR) {
        return parseForStatementNode();
    }
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_WHILE) {
        return parseWhileStatementNode();
    }
    // Return statement
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        return parseReturnStatementNode();  // Use the node-returning version
    }
    // Enum declaration
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_ENUM) {
        return parseEnumDeclaration();
    }
    // Break statement
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_BREAK) {
        return parseBreakStatement();
    }
    // Switch statement
    if (currentToken().type == Lexer::Tokens::Type::KEYWORD_SWITCH) {
        return parseSwitchStatement();
    }

    // Prefix increment/decrement statement (++$var; or --$var;)
    if (currentToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT &&
        peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        auto        opTok    = consumeToken();  // Consume ++ or --
        auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string baseName = parseIdentifierName(idTok);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");

        // Build assignment: $var = $var OP 1
        auto        lhs   = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName, this->current_filename_, idTok.line_number, idTok.column_number);
        auto        rhs   = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
        std::string binOp = (opTok.value == "++") ? "+" : "-";
        auto assignRhs    = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
        // Return the assignment node
        return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                      std::move(assignRhs), this->current_filename_,
                                                                      idTok.line_number, idTok.column_number);
    }

    // Assignment statement: variable, object member assignment
    // Check for potential assignment targets (variable identifier)
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        // Postfix increment/decrement ($var++ or $var--) needs special handling
        // as it's technically an assignment statement $var = $var + 1
        if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER &&
            peekToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {
            auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string baseName = parseIdentifierName(idTok);
            auto        opTok    = expect(Lexer::Tokens::Type::OPERATOR_INCREMENT);
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");

            // Build the assignment $var = $var OP 1
            std::unique_ptr<Interpreter::ExpressionNode> lhs =
                std::make_unique<Interpreter::IdentifierExpressionNode>(baseName, this->current_filename_, idTok.line_number, idTok.column_number);
            std::unique_ptr<Interpreter::ExpressionNode> rhs =
                std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
            std::string binOp = (opTok.value == "++") ? "+" : "-";
            auto assignRhs = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
            // Return the assignment statement node
            return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                          std::move(assignRhs), this->current_filename_,
                                                                          idTok.line_number, idTok.column_number);
        }
        // If not postfix increment, check for standard assignment
        // Check if it's a standard assignment ($var = ..., $obj->prop = ...)
        size_t lookahead_idx = current_token_index_ + 1;  // Move past the initial var/this token
        // Skip -> identifier sequences
        while (lookahead_idx + 1 < tokens_.size() && tokens_[lookahead_idx].type == Lexer::Tokens::Type::PUNCTUATION &&
               tokens_[lookahead_idx].value == "->" &&
               (tokens_[lookahead_idx + 1].type == Lexer::Tokens::Type::IDENTIFIER ||
                tokens_[lookahead_idx + 1].type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER)) {
            lookahead_idx += 2;
        }

        bool is_assignment =
            (lookahead_idx < tokens_.size() && tokens_[lookahead_idx].type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

        if (is_assignment) {
            return parseAssignmentStatementNode();  // Parse and return the assignment node
        }
        // If it started with var/this but wasn't postfix or standard assignment,
        // it will fall through to the expression statement parsing below.
    }
    const auto currentTokenType  = currentToken().type;
    const auto currentTokenValue = currentToken().value;
    const auto peekTokenType     = peekToken().type;
    const auto peekTokenValue    = peekToken().value;
    const auto peekToken2Type    = peekToken(2).type;
    const auto peekToken2Value   = peekToken(2).value;
    const auto peekToken3Type    = peekToken(3).type;
    const auto peekToken3Value   = peekToken(3).value;

    // Enum declaration (handled above, but if missed for some reason, could be a fallback)
    if (currentTokenType == Lexer::Tokens::Type::KEYWORD_ENUM) {
        return parseEnumDeclaration();
    }
    // Break statement (handled above)
    if (currentTokenType == Lexer::Tokens::Type::KEYWORD_BREAK) {
        return parseBreakStatement();
    }
    // Switch statement (handled above)
    if (currentTokenType == Lexer::Tokens::Type::KEYWORD_SWITCH) {
        return parseSwitchStatement();
    }
    // <<< ADD VARIABLE DEFINITION CHECK HERE >>>
    // Check if current token is a known type keyword OR a registered class identifier
    bool       is_type_keyword   = (Parser::variable_types.find(currentTokenType) != Parser::variable_types.end());
    bool       is_class_name     = false;

    if (currentTokenType == Lexer::Tokens::Type::IDENTIFIER &&
        peekTokenType == Lexer::Tokens::Type::VARIABLE_IDENTIFIER &&
        peekToken2Type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT &&
        peekToken3Type == Lexer::Tokens::Type::KEYWORD_NEW) {
        is_class_name = true;
    }

    // Handle variable declarations with optional array syntax
    if (is_type_keyword || is_class_name) {
        size_t lookahead_offset = 1; // Start after the type keyword

        // Check for optional array syntax: type[]
        if (peekToken(lookahead_offset).type == Lexer::Tokens::Type::PUNCTUATION &&
            peekToken(lookahead_offset).value == "[" &&
            peekToken(lookahead_offset + 1).type == Lexer::Tokens::Type::PUNCTUATION &&
            peekToken(lookahead_offset + 1).value == "]") {
            lookahead_offset += 2; // Skip past []
        }

        // Check if we have: variable_identifier = ...
        if ((peekToken(lookahead_offset).type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
             peekToken(lookahead_offset).type == Lexer::Tokens::Type::IDENTIFIER) &&
            peekToken(lookahead_offset + 1).type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT) {
            // Parse as variable definition using the new node-returning function
            return parseVariableDefinitionNode();
        }
    }
    // <<< END VARIABLE DEFINITION CHECK >>>

    // Function call (identifier followed directly by '(')
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        // Parse as a function call statement and return the node
        return parseCallStatement();
    }  // Parse an expression
    auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Handle method calls by turning them into method call operations
    if (expr->kind == ParsedExpression::Kind::MethodCall) {
        // Convert method call expression to method call operation
        Interpreter::OperationsFactory::callMethod(expr->lhs->toString(),  // object name or expression
                                                   expr->name,             // method name
                                                   std::move(expr->args),  // arguments
                                                   Symbols::SymbolContainer::instance()->currentScopeName(),
                                                   this->current_filename_, currentToken().line_number,
                                                   currentToken().column_number);
        return nullptr;  // Operation added, no statement node needed
    }

    // For other expressions, create and return an expression statement node
    return std::make_unique<Interpreter::ExpressionStatementNode>(buildExpressionFromParsed(expr),
                                                                  this->current_filename_, currentToken().line_number,
                                                                  currentToken().column_number);
}

// End of parseStatementNode
// Parse next definition or statement
// (Subsequent methods follow)

void Parser::parseFunctionDefinition() {
    expect(Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION);
    Lexer::Tokens::Token     id_token         = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string              func_name        = id_token.value;

    std::vector<Symbols::FunctionParameterInfo> param_infos = parseParameterList();
    Symbols::Variables::Type func_return_type = parseOptionalReturnType();

    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    size_t opening_brace_idx = current_token_index_ - 1;

    const std::string parent_scope_name = Symbols::SymbolContainer::instance()->currentScopeName(); // CAPTURE PARENT SCOPE

    parseBlockInNewScope(opening_brace_idx, func_name);

    Interpreter::OperationsFactory::defineFunction(func_name, param_infos, func_return_type,
                                                parent_scope_name,
                                                this->current_filename_, id_token.line_number, id_token.column_number);
}

// Parse a top-level class definition: class Name { ... }
void Parser::parseClassDefinition() {
    // 'class'
    expect(Lexer::Tokens::Type::KEYWORD_CLASS);
    // Class name
    auto              nameToken = expect(Lexer::Tokens::Type::IDENTIFIER);
    const std::string className = nameToken.value;

    // Get the file namespace
    const std::string fileNs  = Symbols::SymbolContainer::instance()->currentScopeName();
    // Use :: as namespace separator
    const std::string classNs = fileNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + className;

    // Track this class name for parseType to recognize it as CLASS type
    parsed_class_names_.insert(className);
    parsed_class_names_.insert(classNs);  // Also add fully qualified name

    // Create a ClassSymbol in the symbol table
    auto classSymbol = Symbols::SymbolFactory::createClass(className, fileNs);
    Symbols::SymbolContainer::instance()->add(classSymbol);

    // Register class in the classes registry so hasClass() and getClassInfo() work
    Symbols::SymbolContainer::instance()->registerClass(className);

    // Create class scope (automatically enters it)
    Symbols::SymbolContainer::instance()->create(classNs);
    const std::string class_scope_name = Symbols::SymbolContainer::instance()->currentScopeName(); // This is classNs

    // Gather class members (registration happens at interpretation)
    std::vector<Symbols::PropertyInfo> privateProps;
    std::vector<Symbols::PropertyInfo> publicProps;
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
            consumeToken(); // 'const'
            Symbols::PropertyInfo info = parsePropertyInfo(true);
            if (currentAccess == PRIVATE) {
                privateProps.push_back(std::move(info));
            } else {
                publicProps.push_back(std::move(info));
            }
            continue;
        }
        // Property declaration
        if (Parser::variable_types.find(tok.type) != Parser::variable_types.end() ||
            tok.type == Lexer::Tokens::Type::IDENTIFIER) { // Could be a class name
            Symbols::PropertyInfo info = parsePropertyInfo(false);
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

            std::vector<Symbols::FunctionParameterInfo> params = parseParameterList();
            Symbols::Variables::Type returnType = parseOptionalReturnType();

            expect(Lexer::Tokens::Type::PUNCTUATION, "{");
            size_t opening_brace_idx = current_token_index_ - 1;

            parseBlockInNewScope(opening_brace_idx, methodName);

            Symbols::SymbolContainer::instance()->addMethod(className, methodName, returnType, params);
            Interpreter::OperationsFactory::defineMethod(methodName, params, className, returnType,
                                                        class_scope_name, // Use the captured class scope
                                                        this->current_filename_, nameId.line_number, nameId.column_number);

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
    // Find constructor name if exists
    std::string constructorName;
    for (const auto & methodName : methodNames) {
        if (methodName == "construct") {
            constructorName = methodName;  // Store just the method name, not the full qualified name
            break;
        }
    }

    // Enqueue class definition for interpretation
    auto stmt = std::make_unique<Interpreter::ClassDefinitionStatementNode>(
        className, classNs, std::move(privateProps), std::move(publicProps), std::move(methodNames), constructorName,
        this->current_filename_, nameToken.line_number, nameToken.column_number);
    Operations::Container::instance()->add(
        Symbols::SymbolContainer::instance()->currentScopeName(),
        Operations::Operation{ Operations::Type::Declaration, className, std::move(stmt) });
}

// Parse a top-level function call, e.g., foo(arg1, arg2);
std::unique_ptr<Interpreter::StatementNode> Parser::parseCallStatement() {
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
    // Build expression nodes for arguments
    std::vector<std::unique_ptr<Interpreter::ExpressionNode>> exprs;
    exprs.reserve(args.size());
    for (auto & p : args) {
        exprs.push_back(buildExpressionFromParsed(p));
    }
    // Create CallStatementNode
    auto stmt = std::make_unique<Interpreter::CallStatementNode>(func_name, std::move(exprs), this->current_filename_,
                                                                 id_token.line_number, id_token.column_number);
    return stmt;  // RETURN THE NODE
}

// Parse a return statement, e.g., return; or return expression;
void Parser::parseReturnStatement() {
    auto stmt = this->parseReturnStatementNode();
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::Return, "", std::move(stmt) });
}

// Parse a for-in loop over object members
void Parser::parseForStatement() {
    auto stmt = parseForStatementNode();
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::Loop, "", std::move(stmt) });
}

void Parser::parseWhileStatement() {
    auto stmt = parseWhileStatementNode();
    Operations::Container::instance()->add(Symbols::SymbolContainer::instance()->currentScopeName(),
                                           Operations::Operation{ Operations::Type::While, "", std::move(stmt) });
}

// Continue with numeric literal parsing
//
Symbols::ValuePtr Parser::parseNumericLiteral(const std::string & value, bool is_negative,
                                              Symbols::Variables::Type type) {
    try {
        switch (type) {
            case Symbols::Variables::Type::INTEGER:
                {
                    if (value.find('.') != std::string::npos) {
                        throw std::invalid_argument("Floating point value in integer context: " + value);
                    }
                    int v = std::stoi(value);
                    return Symbols::ValuePtr(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::DOUBLE:
                {
                    double v = std::stod(value);
                    return Symbols::ValuePtr(is_negative ? -v : v);
                }
            case Symbols::Variables::Type::FLOAT:
                {
                    float v = std::stof(value);
                    return Symbols::ValuePtr(is_negative ? -v : v);
                }
            default:
                throw std::invalid_argument("Unsupported numeric type");
        }
    } catch (const std::invalid_argument & e) {
        reportError("Invalid numeric literal: " + value + " (" + e.what() + ")");
    } catch (const std::out_of_range & e) {
        reportError("Numeric literal out of range: " + value + " (" + e.what() + ")");
    }

    return Symbols::ValuePtr::null();  // unreachable
}

void Parser::parseBlockInNewScope(size_t opening_brace_idx, const std::string& scope_suffix_name) {
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
        reportError("Unmatched braces in block/body for scope: " + scope_suffix_name );
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
    const std::string new_scope_name = Symbols::SymbolContainer::instance()->currentScopeName() +
                                       Symbols::SymbolContainer::SCOPE_SEPARATOR + scope_suffix_name;
    Symbols::SymbolContainer::instance()->create(new_scope_name);

    // Parse function body in its own parser instance
    Parser innerParser;
    innerParser.parseScript(filtered_tokens, input_string, this->current_filename_);

    // Exit the function scope, restoring to the parent scope (e.g., class scope or global scope)
    Symbols::SymbolContainer::instance()->enterPreviousScope();
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
            auto newTok = consumeToken(); // Consume 'new'
            auto nameTok = expect(Lexer::Tokens::Type::IDENTIFIER); // ClassName token
            std::string className = nameTok.value;

            // Parse constructor arguments using the helper
            std::vector<ParsedExpressionPtr> constructor_arguments = parseExpressionList(
                Lexer::Tokens::Type::PUNCTUATION, "(",
                Lexer::Tokens::Type::PUNCTUATION, ")",
                Symbols::Variables::Type::NULL_TYPE
            );

            // Create an expression representing the object allocation, now with arguments.
            auto new_object_allocation_expr = ParsedExpression::makeNew(
                className,
                constructor_arguments, // Pass the parsed constructor_arguments directly
                this->current_filename_,
                nameTok.line_number,
                nameTok.column_number
            );

            output_queue.push_back(new_object_allocation_expr); // Push the NewExpression directly

            expect_unary = false;
            atStart = false;
            continue; // Continue the while loop of parseParsedExpression
        }
        // Array literal (at start) or dynamic indexing (postfix)
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "[") {
            if (atStart) {
                // Parse array literal using parseExpressionList
                std::vector<ParsedExpressionPtr> elements = parseExpressionList(
                    Lexer::Tokens::Type::PUNCTUATION, "[",
                    Lexer::Tokens::Type::PUNCTUATION, "]",
                    Symbols::Variables::Type::NULL_TYPE
                );
                std::vector<std::pair<std::string, ParsedExpressionPtr>> members;
                for (size_t idx = 0; idx < elements.size(); ++idx) {
                    members.emplace_back(std::to_string(idx), std::move(elements[idx]));
                }
                // Build as object literal (using the location of the '[' token)
                output_queue.push_back(ParsedExpression::makeObject(std::move(members), this->current_filename_,
                                                                    token.line_number, // Use the original '[' token for location
                                                                    token.column_number));
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
            auto accessExpr =
                ParsedExpression::makeBinary("[]", std::move(lhsExpr), std::move(indexExpr), this->current_filename_,
                                             currentToken().line_number, currentToken().column_number);
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
                    // Key must be an identifier or variable identifier
                    if (currentToken().type != Lexer::Tokens::Type::IDENTIFIER &&
                        currentToken().type != Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                        reportError("Expected identifier for object key");
                    }
                    auto        keyToken = consumeToken(); // Consume key token
                    std::string key      = Parser::parseIdentifierName(keyToken);
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
            output_queue.push_back(ParsedExpression::makeObject(
                std::move(members), this->current_filename_, currentToken().line_number, currentToken().column_number));
            expect_unary = false;
            continue;
        }

        // Member access: '->'
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "->") {
            std::string op = token.value;
            applyHigherPrecedenceOperators(op, operator_stack, output_queue);
            operator_stack.push(op);
            consumeToken();  // Consumes '->'

            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                Lexer::Tokens::Token propToken = consumeToken();
                std::string          propName  = parseIdentifierName(propToken);  // Use helper to handle $ prefix removal
                output_queue.push_back(ParsedExpression::makeVariable(propName, this->current_filename_, propToken.line_number, propToken.column_number));
                expect_unary = false;
                atStart      = false;
                continue;
            } else {
                // If no identifier follows, it's an error or needs different handling.
                // For now, setting expect_unary to true, as the original complex operator block would expect an operand.
                // This part might need further refinement based on exact original logic if an error should be reported.
                expect_unary = true;
            }
        }
        // Namespace resolution operator '::'
        else if (token.type == Lexer::Tokens::Type::OPERATOR_NAMESPACE_RESOLUTION) { // Handle '::'
            std::string op = token.value; // Should be "::"
            applyHigherPrecedenceOperators(op, operator_stack, output_queue);
            operator_stack.push(op); // Push "::" onto the operator stack
            consumeToken();          // Consume the "::" token
            expect_unary = true;     // After "::", we expect an identifier (like a member name)
            atStart = false;         // No longer at the start of a sub-expression
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
                applyStackOperator(operator_stack, output_queue);
            }
            if (operator_stack.empty() || operator_stack.top() != "(") { // Should find '('
                Parser::reportError("Mismatched parentheses in expression", token);
            }
            operator_stack.pop(); // Pop the opening parenthesis '('
            expect_unary = false;
        }
        // Method or function call: expression followed by '('
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "(") {
            // If we have a preceding expression, treat as call: pop callee from output_queue
            if (!expect_unary && !output_queue.empty()) {
                // Parse arguments using parseExpressionList
                // currentToken is '(', parseExpressionList will consume it.
                std::vector<ParsedExpressionPtr> args = parseExpressionList(
                    Lexer::Tokens::Type::PUNCTUATION, "(",
                    Lexer::Tokens::Type::PUNCTUATION, ")",
                    Symbols::Variables::Type::NULL_TYPE
                );
                // Build call: callee could be variable or member expression
                auto callee = std::move(output_queue.back());
                output_queue.pop_back();
                ParsedExpressionPtr callExpr;
                if (callee->kind == ParsedExpression::Kind::Binary && callee->op == "->") {
                    // Method call: object->method(args)
                    callExpr = ParsedExpression::makeMethodCall(
                        std::move(callee->lhs), callee->rhs->name, std::move(args), this->current_filename_,
                        currentToken().line_number, currentToken().column_number);
                } else if (callee->kind == ParsedExpression::Kind::Variable) {
                    // Function call
                    callExpr = ParsedExpression::makeCall(callee->name, std::move(args), this->current_filename_,
                                                          currentToken().line_number, currentToken().column_number);
                } else {
                    reportError("Invalid call target");
                }
                output_queue.push_back(std::move(callExpr));
                expect_unary = false;
                atStart      = false;
                continue;
            }
            // Fallback grouping: treat '(' as usual
            operator_stack.push(token.value); // Push "("
            consumeToken(); // Consume "("
            expect_unary = true; // Expect operand after "("
        } else if (token.type == Lexer::Tokens::Type::IDENTIFIER &&
                   peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
            // Parse function call
            std::string func_name = token.value;
            consumeToken();  // consume function name
            // currentToken is now '(', parseExpressionList will consume it.
            std::vector<ParsedExpressionPtr> call_args = parseExpressionList(
                Lexer::Tokens::Type::PUNCTUATION, "(",
                Lexer::Tokens::Type::PUNCTUATION, ")",
                Symbols::Variables::Type::NULL_TYPE
            );
            // Create call expression node with source location
            {
                auto pe = ParsedExpression::makeCall(func_name, std::move(call_args), this->current_filename_,
                                                     token.line_number, token.column_number);
                output_queue.push_back(std::move(pe));
            }
            expect_unary = false;
        } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC ||
                   token.type == Lexer::Tokens::Type::OPERATOR_RELATIONAL ||
                   token.type == Lexer::Tokens::Type::OPERATOR_LOGICAL ||
                   token.type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {  // Include ++/-- operators
            std::string original_op = std::string(token.lexeme);
            std::string fullOp = original_op; // This will be modified to uOp or pOp if necessary
            bool is_unary_increment_op = (token.type == Lexer::Tokens::Type::OPERATOR_INCREMENT && (original_op == "++" || original_op == "--"));

            if (expect_unary) {
                if (Lexer::isUnaryOperator(original_op) || is_unary_increment_op) {
                    if (original_op == "++") fullOp = "u++";
                    else if (original_op == "--") fullOp = "u--";
                    else if (original_op == "+") fullOp = "u+"; // Ensure distinction from binary +
                    else if (original_op == "-") fullOp = "u-"; // Ensure distinction from binary -
                    else fullOp = "u" + original_op; // For operators like '!'
                } else {
                    // This case implies a binary operator was expected to be unary, which is a syntax error.
                    // Example: `a * / b` where `/` is `expect_unary` but not a unary op.
                    // The Shunting-yard algorithm typically handles this by not finding a unary form,
                    // and then `applyHigherPrecedenceOperators` would use its binary precedence.
                    // If it's a true syntax error, other checks or lack of operands would catch it.
                }
            } else if (is_unary_increment_op) { // Postfix: !expect_unary and is_unary_increment_op
                if (original_op == "++") fullOp = "p++";
                else if (original_op == "--") fullOp = "p--";
            }
            // If not unary and not postfix, fullOp remains original_op (binary operator)

            applyHigherPrecedenceOperators(fullOp, operator_stack, output_queue);
            operator_stack.push(fullOp);
            consumeToken();
            expect_unary = true; // After any operator, we generally expect an operand or a prefix unary operator.
        } else if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                   token.type == Lexer::Tokens::Type::KEYWORD ||
                   token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                   token.type == Lexer::Tokens::Type::IDENTIFIER) {
            // Special case for '$this' variable identifier access
            if (token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER && token.value == "$this") {
                if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "->") {
                    // Parse '$this->property' access (new syntax)
                    output_queue.push_back(parseThisPropertyAccess());
                } else {
                    // Simple '$this' reference in method context - normalize to 'this'
                    output_queue.push_back(ParsedExpression::makeVariable("this", this->current_filename_, token.line_number, token.column_number));
                    consumeToken();
                }
            } else if (token.type == Lexer::Tokens::Type::KEYWORD && token.value == "this") {
                // ERROR: Bare 'this' usage is not allowed in new VoidScript syntax
                reportError("Bare 'this' keyword is not allowed. Use '$this' instead for class member access.", token);
            } else if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
                // Regular identifier handling
                output_queue.push_back(ParsedExpression::makeVariable(token.value, this->current_filename_, token.line_number, token.column_number));
                consumeToken();
            } else {
                if (Lexer::pushOperand(token, expected_var_type, output_queue, this->current_filename_) == false) {
                    Parser::reportError("Invalid type", token, "literal or variable");
                }
                consumeToken();
            }

            expect_unary = false;
            atStart      = false;
        } else {
            break;
        }
    }

    // Empty the operator stack
    while (!operator_stack.empty()) {
        // The applyStackOperator method now handles the logic for popping and applying.
        // It also includes checks for mismatched parentheses if '(' were to be on stack here,
        // and operand availability.
        applyStackOperator(operator_stack, output_queue);
    }

    if (output_queue.size() != 1) {
        reportError("Expression could not be parsed cleanly, expected 1 item on output queue, found " + std::to_string(output_queue.size()));
    }

    return std::move(output_queue.back());
}

// New private method to parse object literal expressions
ParsedExpressionPtr Parser::parseObjectLiteralExpression() {
    auto objectToken = currentToken(); // Token for '{' location, consumeToken will be called by the caller if it's an object literal
    consumeToken(); // Consume '{'
    std::vector<std::pair<std::string, ParsedExpressionPtr>> members;

    // Parse members until '}'
    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        while (true) {
            // Optional type tag before key
            Symbols::Variables::Type memberType = Symbols::Variables::Type::UNDEFINED_TYPE;
            // Note: The original code for parsing optional type tag before key was:
            // if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end()) {
            //     memberType = parseType();
            // }
            // This part is omitted as per simplified instruction, assuming keys don't have explicit types for now,
            // or that `parseType()` would be complex to integrate here without full context.
            // If type parsing for keys is needed, it should be re-added here carefully.

            // Key must be an identifier or variable identifier
            // Key must be an identifier or variable identifier
            if (currentToken().type != Lexer::Tokens::Type::IDENTIFIER &&
                currentToken().type != Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
                reportError("Expected identifier for object key");
            }
            auto        keyToken = consumeToken(); // Consume key token
            std::string key      = Parser::parseIdentifierName(keyToken);

            expect(Lexer::Tokens::Type::PUNCTUATION, ":"); // Expect ':' delimiter

            // Parse value expression
            // The original logic for `expectType` based on `memberType` is simplified here.
            // If `memberType` logic were re-added above, `expectType` should use it.
            auto valueExpr  = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            members.emplace_back(key, std::move(valueExpr));

            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                // If there's a comma, check for immediate '}' for trailing comma case
                if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") {
                    break; // Trailing comma, then end of object
                }
                continue; // Next member
            }
            break; // No comma, end of members for this loop or expect '}'
        }
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, "}"); // Expect closing '}'

    // Use location from the opening brace token
    return ParsedExpression::makeObject(
        std::move(members),
        this->current_filename_,
        objectToken.line_number,
        objectToken.column_number
    );
}

void Parser::parseScript(const std::vector<Lexer::Tokens::Token> & tokens, std::string_view input_string,
                         const std::string & filename) {
    ::Parser::Parser::Exception::current_filename_ = filename;
    tokens_                                        = tokens;
    input_str_view_                                = input_string;
    current_token_index_                           = 0;
    current_filename_                              = filename;

    while (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        parseTopLevelStatement();
    }
    if (!isAtEnd() && currentToken().type != Lexer::Tokens::Type::END_OF_FILE) {
        reportError("Unexpected tokens after program end");
    }
}

Symbols::Variables::Type Parser::parseType() {
    const auto & token = currentToken();
    // Direct lookup for type keyword
    auto         it    = Parser::variable_types.find(token.type);
    if (it != Parser::variable_types.end()) {
        // Base type
        Symbols::Variables::Type baseType = it->second;
        consumeToken(); // Consume the type keyword
        // Array type syntax: baseType[] -> treat as OBJECT/array map
        if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
            peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
                consumeToken(); // '['
                consumeToken(); // ']'
            return Symbols::Variables::Type::OBJECT;
        }
        return baseType;
    }
    // User-defined class types: if identifier names a registered class, return CLASS; otherwise OBJECT
    if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
        // Capture the identifier value as potential class name
        const std::string typeName = token.value;

        // First check if this was a class name we parsed during this session
        if (parsed_class_names_.count(typeName) ||
            (Symbols::SymbolContainer::instance()->currentScopeName() + Symbols::SymbolContainer::SCOPE_SEPARATOR + typeName).length() > 0 && // Ensure not empty
            parsed_class_names_.count(Symbols::SymbolContainer::instance()->currentScopeName() + Symbols::SymbolContainer::SCOPE_SEPARATOR + typeName) ) {
            consumeToken(); // Consume class name
             // Check for array type syntax: ClassName[]
            if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
                peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
                    consumeToken(); // '['
                    consumeToken(); // ']'
                return Symbols::Variables::Type::OBJECT;
            }
            return Symbols::Variables::Type::CLASS;
        }

        // Check if this name is a defined class in the symbol container
        auto * symbolContainer = Symbols::SymbolContainer::instance();

        // First try as is (might be a fully qualified name already)
        if (symbolContainer->hasClass(typeName)) {
            consumeToken(); // Consume class name
             // Check for array type syntax: ClassName[]
            if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
                peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
                    consumeToken(); // '['
                    consumeToken(); // ']'
                return Symbols::Variables::Type::OBJECT;
            }
            return Symbols::Variables::Type::CLASS;
        }

        // Then try with current namespace prefix
        std::string currentNs  = symbolContainer->currentScopeName();
        std::string fqTypeName = currentNs.empty() ? typeName : currentNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + typeName;
        if (symbolContainer->hasClass(fqTypeName)) {
            consumeToken(); // Consume class name
             // Check for array type syntax: ClassName[]
            if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
                peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
                    consumeToken(); // '['
                    consumeToken(); // ']'
                return Symbols::Variables::Type::OBJECT;
            }
            return Symbols::Variables::Type::CLASS;
        }

        reportError("Expected type keyword (string, int, double, float or class name), found identifier: " + typeName, token);
    }
    reportError("Expected type keyword (string, int, double, float or class name)", token);
}

Symbols::ValuePtr Parser::parseValue(Symbols::Variables::Type expected_var_type) {
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
            return Symbols::ValuePtr(token.value);
        }
        reportError("Expected string literal value");
    }

    // BOOLEAN type
    if (expected_var_type == Symbols::Variables::Type::BOOLEAN) {
        if (token.type == Lexer::Tokens::Type::KEYWORD && (token.value == "true" || token.value == "false")) {
            consumeToken();
            return Symbols::ValuePtr(token.value == "true");
        }
        reportError("Expected boolean literal value (true or false)");
    }

    // NUMERIC types
    if (expected_var_type == Symbols::Variables::Type::INTEGER ||
        expected_var_type == Symbols::Variables::Type::DOUBLE || expected_var_type == Symbols::Variables::Type::FLOAT) {
        if (token.type == Lexer::Tokens::Type::NUMBER) {
            Symbols::ValuePtr val = parseNumericLiteral(token.value, is_negative, expected_var_type);
            consumeToken();
            return val;
        }

        reportError("Expected numeric literal value");
    }

    reportError("Unsupported variable type encountered during value parsing");
    return Symbols::ValuePtr::null();  // compiler happy
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
    const auto& token_to_consume = tokens_[current_token_index_];
    current_token_index_++;
    return token_to_consume;
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
        throw std::runtime_error("Cannot access token at end of stream.");
    }
    return tokens_[current_token_index_];
}

// Add this helper method to parse a 'this->property' or '$this->property' access as a special case
ParsedExpressionPtr Parser::parseThisPropertyAccess() {
    // Consume '$this' token (only $this is allowed in new syntax)
    auto thisTok = consumeToken();
    if (thisTok.value != "$this") {
        reportError("Invalid 'this' access. Only '$this' syntax is allowed for class member access.", thisTok);
    }
    std::string thisVarName = "this"; // Always normalize $this to this internally

    // Expect '->' token
    expect(Lexer::Tokens::Type::PUNCTUATION, "->");

    // Get property name (with or without $)
    Lexer::Tokens::Token propTok;
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        propTok = consumeToken();
    } else {
        reportError("Expected property name after '$this->'");
    }

    std::string propName = Parser::parseIdentifierName(propTok);

    // Create a variable expression for 'this'
    auto thisExpr = ParsedExpression::makeVariable(thisVarName, this->current_filename_, thisTok.line_number, thisTok.column_number);

    // Create a binary expression for property access using the "->" operator
    auto propExpr = ParsedExpression::makeVariable(propName, this->current_filename_, propTok.line_number, propTok.column_number);
    auto memberExpr = ParsedExpression::makeBinary("->", std::move(thisExpr), std::move(propExpr), this->current_filename_,
                                                   thisTok.line_number, thisTok.column_number);

    return memberExpr;
}

// Parse an assignment statement (variable, object member, 'this' member) and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseAssignmentStatementNode() {
    Lexer::Tokens::Token baseToken;
    std::string          baseName;
    bool                 isThisAssignment = false;

    // Determine base: '$this' variable, or other variable identifier
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        baseToken = consumeToken();  // Consume variable identifier
        baseName  = parseIdentifierName(baseToken);
        if (baseName == "$this") {
            baseName = "this";  // Normalize $this to this
            isThisAssignment = true;
        }
    } else if (currentToken().type == Lexer::Tokens::Type::KEYWORD && currentToken().value == "this") {
        // ERROR: Bare 'this' is not allowed in assignment
        reportError("Bare 'this' keyword is not allowed in assignment. Use '$this' instead for class member access.", currentToken());
        return nullptr;
    } else {
        // This case should ideally not be reached if called correctly after checks
        // But it's useful as a safety check
        reportError("Expected variable name or '$this' at start of assignment", currentToken());
        return nullptr;  // Unreachable
    }

    // Parse potential property path (-> prop1 -> prop2 ...)
    std::vector<std::string> propertyPath;
    int                      lastPropLine = baseToken.line_number;  // Start with base location
    int                      lastPropCol  = baseToken.column_number;
    while (match(Lexer::Tokens::Type::PUNCTUATION, "->")) {
        Lexer::Tokens::Token propTok;
        if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
            currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
            propTok      = consumeToken();
            lastPropLine = propTok.line_number;                    // Update location to the latest property token
            lastPropCol  = propTok.column_number;
            propertyPath.push_back(parseIdentifierName(propTok));  // Use helper
        } else {
            reportError("Expected property name after '->'", currentToken());
            return nullptr;  // Unreachable
        }
    }

    // Consume assignment operator (=, +=, -=, etc.)
    auto opTok = expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

    // Parse RHS expression
    auto rhsExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Build RHS node
    auto rhsNode = buildExpressionFromParsed(rhsExpr);

    // Handle compound assignment (a OP= b  =>  a = a OP b)
    if (opTok.value != "=") {
        // Build LHS expression to get the current value
        std::unique_ptr<Interpreter::ExpressionNode> lhsNode;
        if (isThisAssignment) {
            lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>("this", this->current_filename_, baseToken.line_number, baseToken.column_number);
        } else {
            lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName, this->current_filename_, baseToken.line_number, baseToken.column_number);
        }

        // Apply property path to LHS node if necessary
        int currentPropLine = baseToken.line_number;  // Location for MemberExpressionNode starts at base
        int currentPropCol  = baseToken.column_number;
        for (const auto & prop : propertyPath) {
            // Use the location of the *previous* node or base for the object part,
            // and the location of the current property identifier for the member itself.
            // However, MemberExpressionNode doesn't store separate locations easily.
            // Using the location of the final property access (lastPropLine/Col) for the whole chain.
            lhsNode = std::make_unique<Interpreter::MemberExpressionNode>(
                std::move(lhsNode), prop, this->current_filename_, lastPropLine, lastPropCol);
        }

        // Extract the binary operator part (e.g., "+=" -> "+")
        std::string binOp = opTok.value.substr(0, opTok.value.size() - 1);

        // Create the binary expression: lhs OP rhs
        rhsNode = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhsNode), binOp, std::move(rhsNode));
    }

    // Create the final assignment statement node
    // Use the location of the base variable/this token for the statement itself
    return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::move(propertyPath), std::move(rhsNode),
                                                                  this->current_filename_, baseToken.line_number,
                                                                  baseToken.column_number);
}

std::unique_ptr<Interpreter::StatementNode> Parser::parseReturnStatementNode() {
    auto                                         returnToken = expect(Lexer::Tokens::Type::KEYWORD_RETURN);
    std::unique_ptr<Interpreter::ExpressionNode> exprNode;

    if (!match(Lexer::Tokens::Type::PUNCTUATION, ";")) {
        auto exprPtr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        exprNode = buildExpressionFromParsed(exprPtr);
    }

    auto stmt = std::make_unique<Interpreter::ReturnStatementNode>(std::move(exprNode), this->current_filename_,
                                                                   returnToken.line_number, returnToken.column_number);

    return stmt;
}

void Parser::parseIncludeStatement() {
    auto        includeToken  = expect(Lexer::Tokens::Type::KEYWORD_INCLUDE, "include");
    auto        filenameToken = expect(Lexer::Tokens::Type::STRING_LITERAL);
    std::string filename      = filenameToken.value;

    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Get the base directory of the initial script
    const std::string baseDir = utils::get_parent_directory(current_filename_);

    // Construct the full path to the included file
    std::string fullPath = baseDir + "/" + filename;

    // Read the contents of the included file
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        reportError("Failed to open included file: " + filename, includeToken);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string includedCode = buffer.str();

    const auto currentNs = Symbols::SymbolContainer::instance()->currentScopeName();

    Lexer::Lexer lexer;
    lexer.setKeyWords(Parser::Parser::keywords);
    lexer.addNamespaceInput(currentNs, includedCode);
    auto                              includedTokens         = lexer.tokenizeNamespace(currentNs);
    // Save the current state
    size_t                            saved_token_index      = current_token_index_;
    std::vector<Lexer::Tokens::Token> saved_tokens           = tokens_;
    std::string_view                  saved_input_str_view   = input_str_view_;
    std::string                       saved_current_filename = current_filename_;

    // Parse the included file
    this->parseScript(includedTokens, includedCode, filename);

    // Restore the original state
    current_token_index_ = saved_token_index;
    tokens_              = saved_tokens;
    input_str_view_      = saved_input_str_view;
    current_filename_    = saved_current_filename;
}

void Parser::parseTopLevelStatement() {
    const auto & currentTok = currentToken();
    const auto & token_type = currentTok.type;
    const auto & token_val  = currentTok.value;

    // Top-level keywords
    if (token_type == Lexer::Tokens::Type::KEYWORD_IF) {
        parseIfStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
        parseFunctionDefinition();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        // Top-level return is generally invalid, but parse it and let interpreter potentially error.
        parseReturnStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_FOR) {
        parseForStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_WHILE) {
        parseWhileStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_CLASS) {
        parseClassDefinition();
        // After class definition, we don't need a semicolon - just continue parsing
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_INCLUDE) {
        parseIncludeStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_ENUM) {
        // Enum declaration at top level
        auto enumNode = parseEnumDeclaration();
        if (enumNode) {
             // Similar to how class definitions are handled, add to operations container
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::Declaration, "", std::move(enumNode) }
            );
        }
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_SWITCH) {
        // Switch statement at top level
        auto switchNode = parseSwitchStatement();
        if (switchNode) {
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::ControlFlow, "", std::move(switchNode) } // Assuming ControlFlow type
            );
        }
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_CONST) {
        parseConstVariableDefinition();
    }
    // Variable definition with a type keyword or a class name
    else if ((Parser::variable_types.find(token_type) != Parser::variable_types.end() ||
              (token_type == Lexer::Tokens::Type::IDENTIFIER &&
               (Symbols::SymbolContainer::instance()->hasClass(token_val) ||
                // Try with namespace prefix as well
                Symbols::SymbolContainer::instance()->hasClass(
                    Symbols::SymbolContainer::instance()->currentScopeName() +
                    Symbols::SymbolContainer::SCOPE_SEPARATOR + token_val)))) &&
             peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        // Always treat as variable definition statement at top level
        parseVariableDefinition();
    }
    // Prefix increment/decrement statement (++$var; or --$var;)
    else if (token_type == Lexer::Tokens::Type::OPERATOR_INCREMENT &&
             peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        auto        opTok    = consumeToken();  // Consume ++ or --
        auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string baseName = parseIdentifierName(idTok);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");

        // Build assignment: $var = $var OP 1
        auto        lhs   = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName, this->current_filename_, idTok.line_number, idTok.column_number);
        auto        rhs   = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
        std::string binOp = (opTok.value == "++") ? "+" : "-";
        auto assignRhs    = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
        // Add the assignment operation
        auto stmt         = std::make_unique<Interpreter::AssignmentStatementNode>(
            baseName, std::vector<std::string>(), std::move(assignRhs), this->current_filename_, idTok.line_number,
            idTok.column_number);
        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::Assignment, "", std::move(stmt) });
    }
    // Assignment statement ($var = ..., $var->prop = ...)
    // Also handles postfix increment $var++
    else if (token_type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        bool is_postfix = (token_type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER &&
                           peekToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT);

        if (is_postfix) {
            // Handle postfix increment/decrement directly here as a statement
            auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string baseName = parseIdentifierName(idTok);
            auto        opTok    = expect(Lexer::Tokens::Type::OPERATOR_INCREMENT);  // Consume ++ or --
            expect(Lexer::Tokens::Type::PUNCTUATION, ";");

            // Build the assignment $var = $var OP 1
            std::unique_ptr<Interpreter::ExpressionNode> lhs =
                std::make_unique<Interpreter::IdentifierExpressionNode>(baseName, this->current_filename_, idTok.line_number, idTok.column_number);
            std::unique_ptr<Interpreter::ExpressionNode> rhs =
                std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::ValuePtr(1));
            std::string binOp = (opTok.value == "++") ? "+" : "-";
            auto assignRhs = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
            // Add the assignment operation
            auto stmt      = std::make_unique<Interpreter::AssignmentStatementNode>(
                baseName, std::vector<std::string>(), std::move(assignRhs), this->current_filename_, idTok.line_number,
                idTok.column_number);
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::Assignment, "", std::move(stmt) });
        } else {
            // Check if it's a standard assignment ($var = ..., $obj->prop = ...)
            size_t lookahead_idx = current_token_index_ + 1;  // Move past the initial var/this token
            // Skip -> identifier sequences
            while (lookahead_idx + 1 < tokens_.size() &&
                   tokens_[lookahead_idx].type == Lexer::Tokens::Type::PUNCTUATION &&
                   tokens_[lookahead_idx].value == "->" &&
                   (tokens_[lookahead_idx + 1].type == Lexer::Tokens::Type::IDENTIFIER ||
                    tokens_[lookahead_idx + 1].type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER)) {
                lookahead_idx += 2;
            }

            bool is_assignment = (lookahead_idx < tokens_.size() &&
                                  tokens_[lookahead_idx].type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

            if (is_assignment) {
                auto stmt = parseAssignmentStatementNode();
                if (stmt) {
                    Operations::Container::instance()->add(
                        Symbols::SymbolContainer::instance()->currentScopeName(),
                        Operations::Operation{ Operations::Type::Assignment, "", std::move(stmt) });
                }
            } else {
                // If not postfix and not assignment, assume it's an expression statement (like a method call)
                auto stmtNode = parseStatementNode();  // parseStatementNode handles expression statements
                if (stmtNode) {
                    Operations::Container::instance()->add(
                        Symbols::SymbolContainer::instance()->currentScopeName(),
                        Operations::Operation{ Operations::Type::Expression, "", std::move(stmtNode) });
                } else {
                    reportError("Invalid statement starting with variable or '$this'", currentTok);
                }
            }
        }
    }
    // Handle bare 'this' keyword at top level (should be an error)
    else if (token_type == Lexer::Tokens::Type::KEYWORD_THIS) {
        reportError("Bare 'this' keyword is not allowed. Use '$this' instead for class member access.", currentTok);
    }
    // Function call (identifier followed directly by '(')
    else if (token_type == Lexer::Tokens::Type::IDENTIFIER && peekToken().type == Lexer::Tokens::Type::PUNCTUATION &&
             peekToken().value == "(") {
        // parseCallStatement now returns the node directly
        auto stmtNode = parseCallStatement();
        if (stmtNode) {
            // Add the call statement operation for top-level calls
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::FunctionCall, "",
                                       std::move(stmtNode) });  // Assuming FunctionCall type is appropriate
        } else {
            // Should not happen if parseCallStatement succeeded
            reportError("Failed to parse function call statement", currentTok);
        }
    }
    // If none of the above specific top-level constructs match,
    // try parsing as a general expression statement (e.g., `5;` - valid but useless, or an error).
    else {
        auto stmtNode = parseStatementNode();
        if (stmtNode) {
            // Add the expression statement to operations
            Operations::Container::instance()->add(
                Symbols::SymbolContainer::instance()->currentScopeName(),
                Operations::Operation{ Operations::Type::Expression, "", std::move(stmtNode) });
        } else {
            // If parseStatementNode returned nullptr, it means it couldn't parse anything valid starting here.
            reportError("Unexpected token at beginning of statement", currentTok);
        }
    }
}

// NEW: Parse an enum definition and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseEnumDeclaration() {
    auto enumKeywordToken = expect(Lexer::Tokens::Type::KEYWORD_ENUM);
    auto enumNameToken = expect(Lexer::Tokens::Type::IDENTIFIER);
    std::string enumName = enumNameToken.value;

    expect(Lexer::Tokens::Type::PUNCTUATION, "{");

    std::vector<std::pair<std::string, std::optional<int>>> enumerators;

    // Handle empty enum: enum MyEnum {};
    if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") {
        expect(Lexer::Tokens::Type::PUNCTUATION, "}");
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");
        return std::make_unique<Interpreter::Nodes::Statement::EnumDeclarationNode>(
            current_filename_, // Use Parser's current_filename_
            enumKeywordToken.line_number,
            enumKeywordToken.column_number,
            enumName,
            std::move(enumerators)
        );
    }

    while (true) {
        if (isAtEnd()) {
            reportError("Unterminated enum declaration, missing '}'", enumKeywordToken);
        }

        auto enumeratorNameToken = expect(Lexer::Tokens::Type::IDENTIFIER);
        std::string enumeratorName = enumeratorNameToken.value;
        std::optional<int> enumeratorValue = std::nullopt;

        if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
            bool isNegative = false;
            if (currentToken().type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC && currentToken().value == "-") {
                consumeToken(); // consume '-'
                isNegative = true;
            }
            auto valueToken = expect(Lexer::Tokens::Type::NUMBER);
            try {
                int val = std::stoi(valueToken.value);
                enumeratorValue = isNegative ? -val : val;
            } catch (const std::invalid_argument& ia) {
                reportError("Invalid integer literal for enum value: " + valueToken.value, valueToken);
            } catch (const std::out_of_range& oor) {
                reportError("Integer literal out of range for enum value: " + valueToken.value, valueToken);
            }
        }
        enumerators.emplace_back(enumeratorName, enumeratorValue);

        // After an enumerator, expect either a comma or a closing brace
        if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") {
            expect(Lexer::Tokens::Type::PUNCTUATION, "}"); // Consume the brace
            break; // End of list
        } else if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
            // If it's a comma, check if the next token is '}', allowing trailing comma
            if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") {
                expect(Lexer::Tokens::Type::PUNCTUATION, "}"); // Consume the brace
                break;
            }
            // Otherwise, the loop continues to expect the next enumerator name
        } else {
            reportError("Expected ',' or '}' after enumerator", currentToken());
        }
    }

    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Use location from the 'enum' keyword token
    return std::make_unique<Interpreter::Nodes::Statement::EnumDeclarationNode>(
        current_filename_, // Use Parser's current_filename_
        enumKeywordToken.line_number,
        enumKeywordToken.column_number,
        enumName,
        std::move(enumerators)
    );
}

// NEW: Parse a break statement and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseBreakStatement() {
    auto breakKeywordToken = expect(Lexer::Tokens::Type::KEYWORD_BREAK);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    return std::make_unique<Interpreter::Nodes::Statement::BreakNode>(
        current_filename_, // Use Parser's current_filename_
        breakKeywordToken.line_number,
        breakKeywordToken.column_number
    );
}

// NEW: Parse a switch statement and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseSwitchStatement() {
    auto switchKeywordToken = expect(Lexer::Tokens::Type::KEYWORD_SWITCH);

    expect(Lexer::Tokens::Type::PUNCTUATION, "(");
    // Parse the switch expression. Using NULL_TYPE as expected_var_type to allow any type.
    auto parsedSwitchExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    auto switchExprNode = buildExpressionFromParsed(parsedSwitchExpr);
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    expect(Lexer::Tokens::Type::PUNCTUATION, "{");

    std::vector<Interpreter::Nodes::Statement::SwitchStatementNode::CaseBlock> caseBlocks;
    std::optional<Interpreter::Nodes::Statement::SwitchStatementNode::DefaultBlock> defaultBlockOpt = std::nullopt;
    bool defaultDeclared = false;

    while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}")) {
        if (isAtEnd()) {
            reportError("Unterminated switch statement, missing '}'", switchKeywordToken);
        }

        if (currentToken().type == Lexer::Tokens::Type::KEYWORD_CASE) {
            consumeToken(); // Consume KEYWORD_CASE
            auto parsedCaseExpr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
            auto caseExprNode = buildExpressionFromParsed(parsedCaseExpr);
            expect(Lexer::Tokens::Type::PUNCTUATION, ":");

            std::vector<std::unique_ptr<Interpreter::StatementNode>> caseStatements;
            while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") &&
                   !(currentToken().type == Lexer::Tokens::Type::KEYWORD_CASE) &&
                   !(currentToken().type == Lexer::Tokens::Type::KEYWORD_DEFAULT)) {
                if (isAtEnd()) {
                    reportError("Unterminated case block in switch statement", switchKeywordToken);
                }
                auto stmt = parseStatementNode(); // This should handle break statements
                if (stmt) {
                    caseStatements.push_back(std::move(stmt));
                }
            }
            // caseBlocks.emplace_back(std::move(caseExprNode), std::move(caseStatements));
            Interpreter::Nodes::Statement::SwitchStatementNode::CaseBlock newCaseBlock(
                std::move(caseExprNode),
                std::move(caseStatements)
            );
            caseBlocks.push_back(std::move(newCaseBlock));

        } else if (currentToken().type == Lexer::Tokens::Type::KEYWORD_DEFAULT) {
            consumeToken(); // Consume KEYWORD_DEFAULT
            if (defaultDeclared) {
                reportError("Multiple default blocks in switch statement are not allowed", currentToken());
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ":");
            defaultDeclared = true;

            std::vector<std::unique_ptr<Interpreter::StatementNode>> defaultStatements;
            while (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "}") &&
                   !(currentToken().type == Lexer::Tokens::Type::KEYWORD_CASE) &&
                   !(currentToken().type == Lexer::Tokens::Type::KEYWORD_DEFAULT)) {
                 if (isAtEnd()) {
                    reportError("Unterminated default block in switch statement", switchKeywordToken);
                }
                auto stmt = parseStatementNode();
                if (stmt) {
                    defaultStatements.push_back(std::move(stmt));
                }
            }
            // defaultBlockOpt = Interpreter::Nodes::Statement::SwitchStatementNode::DefaultBlock(std::move(defaultStatements));
            defaultBlockOpt.emplace(std::move(defaultStatements));

        } else {
            reportError("Expected 'case' or 'default' keyword, or '}' to close switch statement", currentToken());
        }
    }

    expect(Lexer::Tokens::Type::PUNCTUATION, "}");
    expect(Lexer::Tokens::Type::PUNCTUATION, ";"); // Expect semicolon after switch block

    return std::make_unique<Interpreter::Nodes::Statement::SwitchStatementNode>(
        current_filename_, // Use Parser's current_filename_
        switchKeywordToken.line_number,
        switchKeywordToken.column_number,
        std::move(switchExprNode),
        std::move(caseBlocks),
        std::move(defaultBlockOpt)
    );
}


// NEW: Parse a variable definition and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseVariableDefinitionNode() {
    Symbols::Variables::Type var_type = this->parseType();  // Add this->

    // Variable name
    Lexer::Tokens::Token id_token;
    if (this->currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||      // Add this->
        this->currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {               // Add this->
        id_token = this->consumeToken();                                              // Add this->
    } else {
        Parser::Parser::reportError("Expected variable name", this->currentToken());  // Add this->
        return nullptr;                                                               // Should not be reached
    }
    std::string var_name = Parser::Parser::parseIdentifierName(id_token);             // Add this->
    const auto  ns       = Symbols::SymbolContainer::instance()->currentScopeName();  // Use current scope

    this->expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=");                      // Add this->

    auto expr         = this->parseParsedExpression(var_type);                        // Add this->
    // Build expression node FIRST
    auto initExprNode = buildExpressionFromParsed(expr);  // No 'this->' needed for static/global helper

    // Consume the semicolon
    this->expect(Lexer::Tokens::Type::PUNCTUATION, ";");  // Add this->

    // Create and return the node
    return std::make_unique<Interpreter::DeclareVariableStatementNode>(var_name, ns, var_type, std::move(initExprNode),
                                                                       this->current_filename_, id_token.line_number,
                                                                       id_token.column_number);
}

std::vector<ParsedExpressionPtr> Parser::parseExpressionList(
    Lexer::Tokens::Type openTokenType,
    const std::string& openTokenValue,
    Lexer::Tokens::Type closeTokenType,
    const std::string& closeTokenValue,
    Symbols::Variables::Type expressionElementType
) {
    expect(openTokenType, openTokenValue); // Consume opening token (e.g., '(' or '[')

    std::vector<ParsedExpressionPtr> expressions;
    // bool firstExpression = true; // Not needed with current logic

    if (!(currentToken().type == closeTokenType && currentToken().value == closeTokenValue)) {
        while (true) {
            expressions.push_back(parseParsedExpression(expressionElementType));
            // firstExpression = false; // Not needed

            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                // If there's a comma, check for immediate closing token to allow trailing commas
                if (currentToken().type == closeTokenType && currentToken().value == closeTokenValue) {
                    break;
                }
                continue; // Expect another expression
            }
            break; // No comma, so list should end or be followed by closing token
        }
    }

    expect(closeTokenType, closeTokenValue); // Consume closing token (e.g., ')' or ']')
    return expressions;
}

void Parser::applyStackOperator(
    std::stack<std::string>& opStack,
    std::vector<ParsedExpressionPtr>& outputQueue
) {
    if (opStack.empty()) {
        // This case should ideally be prevented by the caller's logic.
        reportError("Operator stack empty in applyStackOperator.", currentToken());
    }
    std::string op = opStack.top();
    opStack.pop();

    if (op == "(") {
        // Parentheses should not be applied as operators. '(' is a marker.
        // This implies a mismatch if it's popped here.
        reportError("Mismatched opening parenthesis encountered on operator stack.", currentToken());
    }
    // Note: Closing parentheses ')' are handled directly in parseParsedExpression and should not reach here.

    // Handle unary operators (prefix/postfix)
    if (op == "u-" || op == "u+" || op == "u!" || op == "u++" || op == "u--" || op == "p++" || op == "p--") {
        if (outputQueue.empty()) {
            reportError("Missing operand for unary operator '" + op + "' from stack.", currentToken());
        }
        auto rhs = std::move(outputQueue.back());
        outputQueue.pop_back();
        outputQueue.push_back(Lexer::applyOperator(op, std::move(rhs)));
    } else { // Binary operators
        if (outputQueue.size() < 2) {
            reportError("Missing operands for binary operator '" + op + "' from stack.", currentToken());
        }
        auto rhs = std::move(outputQueue.back());
        outputQueue.pop_back();
        auto lhs = std::move(outputQueue.back());
        outputQueue.pop_back();
        outputQueue.push_back(Lexer::applyOperator(op, std::move(rhs), std::move(lhs)));
    }
}

void Parser::applyHigherPrecedenceOperators(
    const std::string& currentFullOp, // The operator we are currently processing in parseParsedExpression
    std::stack<std::string>& opStack,
    std::vector<ParsedExpressionPtr>& outputQueue
) {
    while (!opStack.empty()) {
        const std::string& topOp = opStack.top();
        if (topOp == "(") { // Stop if we hit an opening parenthesis
            break;
        }

        int topPrecedence = Lexer::getPrecedence(topOp);
        int currentPrecedence = Lexer::getPrecedence(currentFullOp);

        if (topPrecedence > currentPrecedence ||
            (topPrecedence == currentPrecedence && Lexer::isLeftAssociative(topOp))) {
            applyStackOperator(opStack, outputQueue);
        } else {
            break;
        }
    }
}

std::vector<Symbols::FunctionParameterInfo> Parser::parseParameterList() {
    expect(Lexer::Tokens::Type::PUNCTUATION, "("); // Expect and consume '('
    std::vector<Symbols::FunctionParameterInfo> params;

    if (!(currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")")) {
        while (true) {
            Symbols::Variables::Type paramType = parseType(); // Consumes type token
            Lexer::Tokens::Token paramToken = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string paramName = Parser::parseIdentifierName(paramToken);

            params.push_back({paramName, paramType, "", false, false});

            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                // If there's a comma, check for immediate ')' to allow trailing comma
                if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")") {
                    break;
                }
                continue;
            }
            break;
        }
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, ")"); // Expect and consume ')'
    return params;
}

Symbols::Variables::Type Parser::parseOptionalReturnType() {
    const auto& token = currentToken();
    // Check for built-in types
    auto it = Parser::variable_types.find(token.type);
    if (it != Parser::variable_types.end()) {
         Symbols::Variables::Type baseType = it->second;
         consumeToken(); // Consume the type keyword
         // Check for array type syntax: baseType[]
         if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
             peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
             consumeToken(); // '['
             consumeToken(); // ']'
             return Symbols::Variables::Type::OBJECT; // Array types are objects
         }
         return baseType;
    }
    // Check for user-defined class types
    if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
        const std::string typeName = token.value;
        auto* symbolContainer = Symbols::SymbolContainer::instance();
        std::string currentNs = symbolContainer->currentScopeName();
        // Construct fully qualified name for checking, but also check plain name
        std::string fqTypeName = currentNs.empty() ? typeName : currentNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + typeName;

        if (parsed_class_names_.count(typeName) || parsed_class_names_.count(fqTypeName) ||
            symbolContainer->hasClass(typeName) || symbolContainer->hasClass(fqTypeName)) {
            consumeToken(); // Consume class name identifier
            // Array syntax for class return type? e.g. function foo(): MyClass[]
            if (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
                 peekToken(1).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(1).value == "]") {
                 consumeToken(); // '['
                 consumeToken(); // ']'
                 return Symbols::Variables::Type::OBJECT; // Array of class instances
            }
            return Symbols::Variables::Type::CLASS;
        }
    }
    return Symbols::Variables::Type::NULL_TYPE; // Default
}

Symbols::PropertyInfo Parser::parsePropertyInfo(bool isConstProperty) {
    // 'const' keyword is assumed to be consumed by the caller if isConstProperty is true.

    Symbols::Variables::Type propType = parseType();

    Lexer::Tokens::Token idTok;
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        idTok = consumeToken();
    } else {
        reportError("Expected property name in class definition", currentToken());
    }
    std::string propName = Parser::parseIdentifierName(idTok);

    ParsedExpressionPtr defaultValue = nullptr;
    if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
        // Note: isConstProperty implies it must have an initializer,
        // but language syntax might allow const declarations without immediate assignment
        // if they are assigned in constructor. Current code parses optional value.
        // For 'const' properties, the language might enforce an initializer here.
        // This implementation matches the existing flexibility.
        defaultValue = parseParsedExpression(propType);
    } else {
        if (isConstProperty) {
            // Typically, const properties require an initializer at declaration.
            // reportError("Constant property '" + propName + "' must be initialized.", idTok);
            // However, the original code allowed optional default values for consts.
            // We will retain that flexibility unless specified otherwise.
        }
    }

    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // The PropertyInfo struct is {name, type, defaultValueExpr}
    // Add isConst if PropertyInfo needs to store it, or handle by separate lists.
    // Current ClassDefinitionStatementNode takes separate lists for private/public, not const-ness directly in PropertyInfo.
    return {propName, propType, std::move(defaultValue)};
}

}  // namespace Parser
