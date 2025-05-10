#include "Parser/Parser.hpp"

#include "Lexer/Lexer.hpp"
// Static filename for unified error reporting in Parser::Exception
std::string Parser::Parser::Exception::current_filename_;

#include <fstream>
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
#include "ParsedExpression.hpp"
#include "Parser.hpp"
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
    { "include",  Lexer::Tokens::Type::KEYWORD_INCLUDE              },
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

    // Create a constant definition statement node
    auto stmt = std::make_unique<Interpreter::DeclareVariableStatementNode>(
        var_name, ns, var_type, buildExpressionFromParsed(expr), current_filename_, id_token.line_number,
        id_token.column_number, /*isConst=*/true);

    // Add the operation to the container
    Operations::Container::instance()->add(
        ns, Operations::Operation{ Operations::Type::Declaration, var_name, std::move(stmt) });

    // Also directly add the constant to the symbol table for immediate access
    // Create a constant symbol directly
    auto                     exprNode = buildExpressionFromParsed(expr);
    Interpreter::Interpreter tempInterpreter(false);
    try {
        Symbols::Value value  = exprNode->evaluate(tempInterpreter);
        auto           symbol = Symbols::SymbolFactory::createConstant(var_name, value, ns);
        Symbols::SymbolContainer::instance()->defineInScope(ns, symbol);
    } catch (const std::exception & e) {
        throw std::runtime_error("Failed to evaluate constant expression: " + std::string(e.what()));
    }

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
    // Corrected: ns should be the pure scope name, not combined with sub-namespace here.
    const auto ns = Symbols::SymbolContainer::instance()->currentScopeName();

    // Check if there's an initializer
    std::unique_ptr<Interpreter::ExpressionNode> initExprNode;
    Symbols::Value                               value;
    if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
        auto expr                         = parseParsedExpression(var_type);
        // Build expression node FIRST
        initExprNode                      = buildExpressionFromParsed(expr);
        // Create a variable symbol directly
        auto                     exprNode = buildExpressionFromParsed(expr);
        Interpreter::Interpreter tempInterpreter(false);
        value = exprNode->evaluate(tempInterpreter);
    } else {
        // Create a default value based on the type
        switch (var_type) {
            case Symbols::Variables::Type::INTEGER:
                value        = Symbols::Value(0);
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
            case Symbols::Variables::Type::DOUBLE:
            case Symbols::Variables::Type::FLOAT:
                value        = Symbols::Value(0.0);
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
            case Symbols::Variables::Type::STRING:
                value        = Symbols::Value("");
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
            case Symbols::Variables::Type::BOOLEAN:
                value        = Symbols::Value(false);
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
            case Symbols::Variables::Type::OBJECT:
            case Symbols::Variables::Type::CLASS:
                value        = Symbols::Value();
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
            default:
                value        = Symbols::Value();
                initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
                break;
        }
    }

    // Create a variable definition statement node
    auto stmt = std::make_unique<Interpreter::DeclareVariableStatementNode>(
        var_name, ns, var_type, std::move(initExprNode), current_filename_, id_token.line_number,
        id_token.column_number);

    // Add the operation to the container
    Operations::Container::instance()->add(
        ns, Operations::Operation{ Operations::Type::Declaration, var_name, std::move(stmt) });

    // Also directly add the variable to the symbol table for immediate access
    auto symbol = Symbols::SymbolFactory::createVariable(var_name, value, ns, var_type);
    Symbols::SymbolContainer::instance()->defineInScope(ns, symbol);

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

    // --- Create loop scope ONCE at the beginning ---
    const std::string currentScope = Symbols::SymbolContainer::instance()->currentScopeName();
    //const std::string loopScope =
    //        currentScope + "::for_" + std::to_string(forToken.line_number) + "_" + std::to_string(forToken.column_number);
    //Symbols::SymbolContainer::instance()->create(loopScope);  // Create & Enter loopScope
    // --- End scope creation ---

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
    // >>> Remove scope creation from here
    /*
    const std::string currentScope = Symbols::SymbolContainer::instance()->currentScopeName();
    const std::string loopScope = currentScope + "::for_" + std::to_string(forToken.line_number) + "_" + std::to_string(forToken.column_number);
    Symbols::SymbolContainer::instance()->create(loopScope); // Create & Enter
    */
    // <<< End removal
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

    // Prefix increment/decrement statement (++$var; or --$var;)
    if (currentToken().type == Lexer::Tokens::Type::OPERATOR_INCREMENT &&
        peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        auto        opTok    = consumeToken();  // Consume ++ or --
        auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string baseName = parseIdentifierName(idTok);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");

        // Build assignment: $var = $var OP 1
        auto        lhs   = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
        auto        rhs   = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
        std::string binOp = (opTok.value == "++") ? "+" : "-";
        auto assignRhs    = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
        // Return the assignment node
        return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                      std::move(assignRhs), this->current_filename_,
                                                                      idTok.line_number, idTok.column_number);
    }

    // Assignment statement: variable, object member, or 'this' member assignment
    // Check for potential assignment targets (variable identifier or 'this')
    if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "this")) {
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
                std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
            std::unique_ptr<Interpreter::ExpressionNode> rhs =
                std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
            std::string binOp = (opTok.value == "++") ? "+" : "-";
            auto assignRhs = std::make_unique<Interpreter::BinaryExpressionNode>(std::move(lhs), binOp, std::move(rhs));
            // Return the assignment statement node
            return std::make_unique<Interpreter::AssignmentStatementNode>(baseName, std::vector<std::string>(),
                                                                          std::move(assignRhs), this->current_filename_,
                                                                          idTok.line_number, idTok.column_number);
        }
        // If not postfix increment, check for standard assignment
        else {
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
                // If not postfix and not assignment, try parsing as an expression statement
                auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                if (expr) {
                    auto stmtNode = std::make_unique<Interpreter::ExpressionStatementNode>(
                        buildExpressionFromParsed(expr), this->current_filename_, currentToken().line_number,
                        currentToken().column_number);

                    Operations::Container::instance()->add(
                        Symbols::SymbolContainer::instance()->currentScopeName(),
                        Operations::Operation{ Operations::Type::Expression, "", std::move(stmtNode) });
                } else {
                    reportError("Invalid statement starting with variable or 'this'", currentToken());
                }
            }
        }
    }

    // <<< ADD VARIABLE DEFINITION CHECK HERE >>>
    // Check if current token is a known type keyword OR a registered class identifier
    bool is_type_keyword = (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end());
    bool is_class_name   = (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
                          Modules::UnifiedModuleManager::instance().hasClass(currentToken().value));

    if ((is_type_keyword || is_class_name) && (peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                                               peekToken().type == Lexer::Tokens::Type::IDENTIFIER)) {
        // Always parse as variable definition if we have a type followed by identifier
        // This ensures we handle method calls and other complex expressions in initializers
        auto stmt = parseVariableDefinitionNode();
        if (stmt) {
            return stmt;  // Return the statement node directly
        }
    }
    // <<< END VARIABLE DEFINITION CHECK >>>

    // Function call (identifier followed directly by '(')
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        std::string func_name = currentToken().value;
        // parseCallStatement now handles adding the operation internally
        auto        stmtNode  = parseCallStatement();
        if (!stmtNode) {
            reportError("Failed to parse function call statement", currentToken());
        }

        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::FunctionCall, func_name, std::move(stmtNode) });
    }

    // If none of the above specific constructs match,
    // try parsing as a general expression statement
    auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Create and return an expression statement node
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
    Symbols::Variables::Type func_return_type = Symbols::Variables::Type::NULL_TYPE;
    expect(Lexer::Tokens::Type::PUNCTUATION, "(");

    Symbols::FunctionParameterInfo param_infos;

    if (currentToken().type != Lexer::Tokens::Type::PUNCTUATION || currentToken().value != ")") {
        while (true) {
            Symbols::Variables::Type param_type     = parseType();
            Lexer::Tokens::Token     param_id_token = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
            std::string              param_name     = param_id_token.value;
            if (!param_name.empty() && param_name[0] == '$') {
                param_name = param_name.substr(1);
            }
            param_infos.push_back({ param_name, param_type });
            if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                continue;
            }
            if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == ")") {
                break;
            }
            reportError("Expected ',' or ')' in parameter list");
        }
    }
    expect(Lexer::Tokens::Type::PUNCTUATION, ")");

    // Check for return type before the opening brace
    if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end() ||
        currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        func_return_type = parseType();
    }

    expect(Lexer::Tokens::Type::PUNCTUATION, "{");
    size_t      opening_brace_idx = current_token_index_ - 1;
    std::string currentScope      = Symbols::SymbolContainer::instance()->currentScopeName();
    // Register the function symbol only (do not evaluate body)
    auto        funcSymbol =
        Symbols::SymbolFactory::createFunction(func_name, currentScope, param_infos, "", func_return_type);
    Symbols::SymbolContainer::instance()->defineInScope(currentScope, funcSymbol);
    // Parse the function/method body using the brace index (do not evaluate at parse time)
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
    Modules::UnifiedModuleManager::instance().registerClass(className);
    // Enter class scope for methods and parsing
    const std::string fileNs  = Symbols::SymbolContainer::instance()->currentScopeName();
    // Use :: as namespace separator
    const std::string classNs = fileNs + "::" + className;
    // Create class scope (automatically enters it)
    Symbols::SymbolContainer::instance()->create(classNs);
    // Gather class members (registration happens at interpretation)
    std::vector<Modules::ClassInfo::PropertyInfo> privateProps;
    std::vector<Modules::ClassInfo::PropertyInfo> publicProps;
    std::vector<Modules::ClassInfo::MethodInfo>   privateMethods;
    std::vector<Modules::ClassInfo::MethodInfo>   publicMethods;

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
            Modules::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue),
                                                   (currentAccess == PUBLIC) };
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
            Modules::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue),
                                                   (currentAccess == PUBLIC) };
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

                    // Add parameter to method scope immediately
                    auto paramSymbol = Symbols::SymbolFactory::createVariable(
                        paramName, Symbols::Value(), Symbols::SymbolContainer::instance()->currentScopeName(),
                        paramType);
                    Symbols::SymbolContainer::instance()->defineInScope(
                        Symbols::SymbolContainer::instance()->currentScopeName(), paramSymbol);

                    if (match(Lexer::Tokens::Type::PUNCTUATION, ",")) {
                        continue;
                    }
                    break;
                }
            }
            expect(Lexer::Tokens::Type::PUNCTUATION, ")");
            // Optional return type
            Symbols::Variables::Type returnType = Symbols::Variables::Type::NULL_TYPE;
            if (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end() ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                returnType = parseType();
            }
            // Parse and record method body
            expect(Lexer::Tokens::Type::PUNCTUATION, "{");
            size_t opening_brace_idx = current_token_index_ - 1;
            parseFunctionBody(opening_brace_idx, methodName, returnType, params);

            // Create a MethodInfo object and add it to the appropriate list
            Modules::ClassInfo::MethodInfo methodInfo{ methodName, returnType, (currentAccess == PUBLIC) };

            if (currentAccess == PRIVATE) {
                privateMethods.push_back(methodInfo);
            } else {
                publicMethods.push_back(methodInfo);
            }
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
        className, std::move(privateProps), std::move(publicProps), std::move(publicMethods), std::move(privateMethods),
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

    // Check if this is a class method by looking for a double-colon in the current scope name
    std::string currentScope = Symbols::SymbolContainer::instance()->currentScopeName();
    if (currentScope.find("::") != std::string::npos) {
        // This might be a class method, add 'this' variable to the method scope
        addThisToMethodScope(newns);
    }

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
            output_queue.push_back(ParsedExpression::makeNew(className, std::move(args), this->current_filename_,
                                                             currentToken().line_number, currentToken().column_number));
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
                output_queue.push_back(ParsedExpression::makeObject(std::move(members), this->current_filename_,
                                                                    currentToken().line_number,
                                                                    currentToken().column_number));
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
            output_queue.push_back(ParsedExpression::makeObject(
                std::move(members), this->current_filename_, currentToken().line_number, currentToken().column_number));
            expect_unary = false;
            continue;
        }

        // Member access: '->'
        if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.lexeme == "->") {
            consumeToken();  // consume ->

            // Check if the next token is a variable identifier with a $ prefix (e.g., this->$property)
            if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
                Lexer::Tokens::Token propToken = consumeToken();
                std::string          propName  = propToken.value;
                // Remove $ prefix from property name if present
                if (!propName.empty() && propName[0] == '$') {
                    propName = propName.substr(1);
                }

                // Get the object expression from the output queue
                if (output_queue.empty()) {
                    reportError("Missing object for member access");
                }
                auto objExpr = std::move(output_queue.back());
                output_queue.pop_back();

                // Create member access expression
                auto memberExpr =
                    ParsedExpression::makeMember(std::move(objExpr), propName, this->current_filename_,
                                                 currentToken().line_number, currentToken().column_number);
                output_queue.push_back(std::move(memberExpr));

                expect_unary = false;
                atStart      = false;

                // If next token is '(', parse method call
                if (currentToken().type == Lexer::Tokens::Type::PUNCTUATION && currentToken().value == "(") {
                    consumeToken();  // consume '('

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

                    // Get the object expression from the output queue
                    if (output_queue.empty()) {
                        reportError("Missing object for method call");
                    }
                    auto objExpr = std::move(output_queue.back());
                    output_queue.pop_back();

                    // Create method call expression directly
                    auto callExpr = ParsedExpression::makeMethodCall(
                        std::move(objExpr), propName, std::move(args), this->current_filename_,
                        currentToken().line_number, currentToken().column_number);
                    output_queue.push_back(std::move(callExpr));
                }
                continue;
            }
            reportError("Expected property name after '->'");

        }
        // Method or function call: expression followed by '('
        else if (token.type == Lexer::Tokens::Type::PUNCTUATION && token.value == "(") {
            // If we have a preceding expression, treat as call
            if (!expect_unary && !output_queue.empty()) {
                // Process any pending operators before handling the call
                while (!operator_stack.empty() && operator_stack.top() != "(") {
                    std::string op = operator_stack.top();
                    operator_stack.pop();

                    if (op == "u-" || op == "u+" || op == "u!" || op == "u++" || op == "u--" || op == "p++" ||
                        op == "p--") {
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

                // Build call expression
                auto callee = std::move(output_queue.back());
                output_queue.pop_back();
                ParsedExpressionPtr callExpr;

                if (callee->kind == ParsedExpression::Kind::Member) {
                    // Method call: object->method(args)
                    callExpr = ParsedExpression::makeMethodCall(std::move(callee->lhs), callee->name, std::move(args),
                                                                this->current_filename_, currentToken().line_number,
                                                                currentToken().column_number);
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
            operator_stack.push(token.value);
            consumeToken();
            expect_unary = true;
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
                auto pe = ParsedExpression::makeCall(func_name, std::move(call_args), this->current_filename_,
                                                     token.line_number, token.column_number);
                output_queue.push_back(std::move(pe));
            }
            expect_unary = false;
        } else if (token.type == Lexer::Tokens::Type::OPERATOR_ARITHMETIC ||
                   token.type == Lexer::Tokens::Type::OPERATOR_RELATIONAL ||
                   token.type == Lexer::Tokens::Type::OPERATOR_LOGICAL ||
                   token.type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {  // Include ++/-- operators
            std::string op                 = std::string(token.lexeme);
            std::string original_op        = op;  // Keep original for error messages if needed
            bool        is_unary_increment = (token.type == Lexer::Tokens::Type::OPERATOR_INCREMENT);

            if (expect_unary && (Lexer::isUnaryOperator(op) || is_unary_increment)) {  // Handle unary +, -, !, ++, --
                // Distinguish prefix increment/decrement
                if (op == "++") {
                    op = "u++";
                } else if (op == "--") {
                    op = "u--";
                } else {
                    op = "u" + op;  // u+, u-, u!
                }
            } else if (!expect_unary && is_unary_increment) {
                // Distinguish postfix increment/decrement
                if (op == "++") {
                    op = "p++";
                } else if (op == "--") {
                    op = "p--";
                }
            }  // Else, it's a binary operator +, -, *, /, %, ==, !=, <, >, <=, >=, &&, ||

            // Process operators on the stack with higher precedence
            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                if (top == "(" ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) >= Lexer::getPrecedence(top)) ||
                    (Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) > Lexer::getPrecedence(top))) {
                    break;
                }

                operator_stack.pop();
                if (top == "u-" || top == "u+" || top == "u!" || top == "u++" || top == "u--" || top == "p++" ||
                    top == "p--") {  // Unary ops
                    if (output_queue.empty()) {
                        Parser::reportError("Missing operand for unary operator '" + top + "'", token);
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(top, std::move(rhs)));
                } else {
                    // Binary operators
                    if (output_queue.size() < 2) {
                        Parser::reportError("Missing operands for binary operator '" + top + "'", token);
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    auto lhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(top, std::move(rhs), std::move(lhs)));
                }
            }

            operator_stack.push(op);
            consumeToken();
            expect_unary = true;
            continue;
        } else if (token.type == Lexer::Tokens::Type::NUMBER || token.type == Lexer::Tokens::Type::STRING_LITERAL ||
                   token.type == Lexer::Tokens::Type::KEYWORD ||
                   token.type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                   token.type == Lexer::Tokens::Type::IDENTIFIER) {
            // Special case for 'this->$property' syntax
            if (token.type == Lexer::Tokens::Type::IDENTIFIER && token.value == "this" &&
                peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "->") {
                // Use helper method to parse 'this->$property' access
                output_queue.push_back(parseThisPropertyAccess());
            } else if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
                // Special case for 'this' keyword in method contexts
                if (token.value == "this") {
                    // Check if we're in a method context by looking for double-colon in the current scope name
                    std::string currentScope = Symbols::SymbolContainer::instance()->currentScopeName();
                    if (currentScope.find("::") != std::string::npos) {
                        // If 'this' doesn't exist in the current scope, create a dummy one
                        auto * sc         = Symbols::SymbolContainer::instance();
                        auto   thisSymbol = sc->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, "this");
                        if (!thisSymbol) {
                            // Create a dummy 'this' object with __class__ metadata
                            Symbols::Value::ObjectMap thisObj;
                            // Extract class name from scope (format: file::className::methodName)
                            std::string               className;
                            size_t                    lastColonPos = currentScope.rfind("::");
                            if (lastColonPos != std::string::npos) {
                                size_t prevColonPos = currentScope.rfind("::", lastColonPos - 1);
                                if (prevColonPos != std::string::npos) {
                                    className = currentScope.substr(prevColonPos + 2, lastColonPos - prevColonPos - 2);
                                }
                            }

                            if (!className.empty()) {
                                thisObj["__class__"] = Symbols::Value(className);
                                sc->add(Symbols::SymbolFactory::createVariable("this", Symbols::Value(thisObj),
                                                                               currentScope));
                            }
                        }
                    }
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
            atStart      = false;
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

        // Handle unary operators (prefix/postfix)
        if (op == "u-" || op == "u+" || op == "u!" || op == "u++" || op == "u--" || op == "p++" || op == "p--") {
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

    // Store current scope to restore later
    std::string current_scope = Symbols::SymbolContainer::instance()->currentScopeName();

    // Create and enter a new scope for this script
    std::string script_ns = filename;
    Symbols::SymbolContainer::instance()->create(script_ns);
    Symbols::SymbolContainer::instance()->enter(script_ns);

    // Parse statements until we reach EOF
    while (!isAtEnd()) {
        if (currentToken().type == Lexer::Tokens::Type::END_OF_FILE) {
            break;
        }
        parseTopLevelStatement();
    }

    // Get all operations from this script
    auto script_operations = Operations::Container::instance()->getAll(script_ns);

    // Move all operations from script scope to current scope
    for (const auto & operation : script_operations) {
        Operations::Container::instance()->add(current_scope, std::move(*operation));
    }

    // Move all symbols from script scope to current scope
    auto script_symbols = Symbols::SymbolContainer::instance()->getAll(script_ns);
    for (const auto & symbol : script_symbols) {
        Symbols::SymbolContainer::instance()->defineInScope(current_scope, symbol);
    }

    // Clear operations from the script scope
    Operations::Container::instance()->clear(script_ns);

    // Exit script scope and return to current scope
    Symbols::SymbolContainer::instance()->enterPreviousScope();
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
    // User-defined class types: if identifier names a registered class, return CLASS
    if (token.type == Lexer::Tokens::Type::IDENTIFIER) {
        // Capture the identifier value as potential class name
        const std::string typeName = token.value;
        // Consume the identifier token
        consumeToken();
        // If this name is a defined class, treat as CLASS
        if (Modules::UnifiedModuleManager::instance().hasClass(typeName)) {
            return Symbols::Variables::Type::CLASS;
        }
        // If not a class, report error
        reportError("Expected type keyword (string, int, double, float) or class name, got: " + typeName);
    }
    // If we get here, the token is neither a type keyword nor a valid class name
    reportError("Expected type keyword (string, int, double, float) or class name, got: " + token.dump());
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
    return current_token_index_ >= tokens_.size() ||
           (current_token_index_ < tokens_.size() &&
            tokens_[current_token_index_].type == Lexer::Tokens::Type::END_OF_FILE);
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
        if (!tokens_.empty()) {
            return tokens_.back();  // Return the last token (should be EOF)
        }
        throw std::runtime_error("Token stream is empty");
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
    auto memberExpr = ParsedExpression::makeMember(std::move(thisExpr), propName, this->current_filename_,
                                                   thisTok.line_number, thisTok.column_number);

    return memberExpr;
}

// Helper to parse an identifier name, stripping leading '$' if present
std::string Parser::parseIdentifierName(const Lexer::Tokens::Token & token) {
    std::string name = token.value;
    if (!name.empty() && name[0] == '$') {
        return name.substr(1);
    }
    return name;
}

// Parse an assignment statement (variable, object member, 'this' member) and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseAssignmentStatementNode() {
    // Parse the left-hand side (target of assignment)
    auto target = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    if (!target) {
        reportError("Invalid assignment target", currentToken());
        return nullptr;
    }

    // Expect assignment operator
    expect(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

    // Parse the right-hand side (value being assigned)
    auto value = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
    if (!value) {
        reportError("Invalid assignment value", currentToken());
        return nullptr;
    }

    // Expect semicolon
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Extract target name and property path from the target expression
    std::string              targetName;
    std::vector<std::string> propertyPath;

    if (target->kind == ParsedExpression::Kind::Variable) {
        targetName = target->name;
    } else if (target->kind == ParsedExpression::Kind::Binary && target->op == "->") {
        // Handle member access: obj->prop
        if (target->lhs->kind == ParsedExpression::Kind::Variable) {
            targetName = target->lhs->name;
            if (target->rhs->kind == ParsedExpression::Kind::Variable) {
                propertyPath.push_back(target->rhs->name);
            }
        }
    }

    // Create and return the assignment statement node
    return std::make_unique<Interpreter::AssignmentStatementNode>(
        targetName, propertyPath, buildExpressionFromParsed(value), this->current_filename_, currentToken().line_number,
        currentToken().column_number);
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

void Parser::parseTopLevelStatement() {
    const auto & currentTok = currentToken();
    auto         token_type = currentTok.type;
    auto         token_val  = currentTok.value;

    // Constant variable definition
    if (token_type == Lexer::Tokens::Type::KEYWORD_CONST) {
        parseConstVariableDefinition();
    }
    // Variable type keyword (int, string, etc.)
    else if (Parser::variable_types.find(token_type) != Parser::variable_types.end() ||
             (token_type == Lexer::Tokens::Type::IDENTIFIER &&
              Modules::UnifiedModuleManager::instance().hasClass(token_val))) {
        parseVariableDefinition();
    }
    // Function declaration
    else if (token_type == Lexer::Tokens::Type::KEYWORD_FUNCTION_DECLARATION) {
        parseFunctionDefinition();
    }
    // Class declaration
    else if (token_type == Lexer::Tokens::Type::KEYWORD_CLASS) {
        parseClassDefinition();
    }
    // If statement
    else if (token_type == Lexer::Tokens::Type::KEYWORD_IF) {
        parseIfStatement();
    }
    // While loop
    else if (token_type == Lexer::Tokens::Type::KEYWORD_WHILE) {
        parseWhileStatement();
    }
    // For loop
    else if (token_type == Lexer::Tokens::Type::KEYWORD_FOR) {
        parseForStatement();
    }
    // Return statement
    else if (token_type == Lexer::Tokens::Type::KEYWORD_RETURN) {
        parseReturnStatement();
    }
    // Include statement
    else if (token_type == Lexer::Tokens::Type::KEYWORD_INCLUDE) {
        parseIncludeStatement();
    }
    // Prefix increment/decrement
    else if (token_type == Lexer::Tokens::Type::OPERATOR_INCREMENT) {
        // Handle prefix increment/decrement directly here as a statement
        auto        opTok    = expect(Lexer::Tokens::Type::OPERATOR_INCREMENT);  // Consume ++ or --
        auto        idTok    = expect(Lexer::Tokens::Type::VARIABLE_IDENTIFIER);
        std::string baseName = parseIdentifierName(idTok);
        expect(Lexer::Tokens::Type::PUNCTUATION, ";");

        // Build the assignment $var = $var OP 1
        std::unique_ptr<Interpreter::ExpressionNode> lhs =
            std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
        auto        rhs   = std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
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
    // Assignment statement ($var = ..., $var->prop = ..., this->prop = ...)
    // Also handles postfix increment $var++
    else if ((token_type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) ||
             (token_type == Lexer::Tokens::Type::IDENTIFIER && token_val == "this")) {
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
                std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
            std::unique_ptr<Interpreter::ExpressionNode> rhs =
                std::make_unique<Interpreter::LiteralExpressionNode>(Symbols::Value(1));
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
                // If not postfix and not assignment, try parsing as an expression statement
                auto expr = parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
                if (expr) {
                    auto stmtNode = std::make_unique<Interpreter::ExpressionStatementNode>(
                        buildExpressionFromParsed(expr), this->current_filename_, currentToken().line_number,
                        currentToken().column_number);

                    Operations::Container::instance()->add(
                        Symbols::SymbolContainer::instance()->currentScopeName(),
                        Operations::Operation{ Operations::Type::Expression, "", std::move(stmtNode) });
                } else {
                    reportError("Invalid statement starting with variable or 'this'", currentTok);
                }
            }
        }
    }
    // Function call (identifier followed directly by '(')
    else if (token_type == Lexer::Tokens::Type::IDENTIFIER && peekToken().type == Lexer::Tokens::Type::PUNCTUATION &&
             peekToken().value == "(") {
        std::string func_name = currentToken().value;
        // parseCallStatement now handles adding the operation internally
        auto        stmtNode  = parseCallStatement();
        if (!stmtNode) {
            reportError("Failed to parse function call statement", currentTok);
        }

        Operations::Container::instance()->add(
            Symbols::SymbolContainer::instance()->currentScopeName(),
            Operations::Operation{ Operations::Type::FunctionCall, func_name, std::move(stmtNode) });
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

// NEW: Parse a variable definition and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseVariableDefinitionNode() {
    // Parse type
    Symbols::Variables::Type var_type = this->parseType();

    // Variable name
    Lexer::Tokens::Token id_token;
    if (this->currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
        this->currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {
        id_token = this->consumeToken();
    } else {
        this->reportError("Expected variable name", this->currentToken());
        return nullptr;
    }
    std::string var_name = this->parseIdentifierName(id_token);
    const auto  ns       = Symbols::SymbolContainer::instance()->currentScopeName();

    // Check if there's an initializer
    std::unique_ptr<Interpreter::ExpressionNode> initExprNode;

    if (match(Lexer::Tokens::Type::OPERATOR_ASSIGNMENT, "=")) {
        // Parse the initializer expression with NULL_TYPE to allow any valid expression
        // This includes method calls, object member access, etc.
        auto expr = this->parseParsedExpression(Symbols::Variables::Type::NULL_TYPE);
        if (!expr) {
            this->reportError("Invalid initializer expression", this->currentToken());
            return nullptr;
        }
        initExprNode = buildExpressionFromParsed(expr);
    } else {
        // Create a default value based on the type
        Symbols::Value value;
        switch (var_type) {
            case Symbols::Variables::Type::INTEGER:
                value = Symbols::Value(0);
                break;
            case Symbols::Variables::Type::DOUBLE:
            case Symbols::Variables::Type::FLOAT:
                value = Symbols::Value(0.0);
                break;
            case Symbols::Variables::Type::STRING:
                value = Symbols::Value("");
                break;
            case Symbols::Variables::Type::BOOLEAN:
                value = Symbols::Value(false);
                break;
            case Symbols::Variables::Type::OBJECT:
            case Symbols::Variables::Type::CLASS:
                value = Symbols::Value();
                break;
            default:
                value = Symbols::Value();
                break;
        }
        initExprNode = std::make_unique<Interpreter::LiteralExpressionNode>(value);
    }

    // Consume the semicolon
    this->expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Create and return the node
    return std::make_unique<Interpreter::DeclareVariableStatementNode>(var_name, ns, var_type, std::move(initExprNode),
                                                                       this->current_filename_, id_token.line_number,
                                                                       id_token.column_number);
}

void Parser::parseIncludeStatement() {
    // Consume the 'include' keyword
    expect(Lexer::Tokens::Type::KEYWORD_INCLUDE);

    // Expect a string literal with the filename
    auto        filename_token = expect(Lexer::Tokens::Type::STRING_LITERAL);
    std::string filename       = filename_token.value;

    // Expect semicolon
    expect(Lexer::Tokens::Type::PUNCTUATION, ";");

    // Handle relative paths by resolving against the current script's directory
    std::string resolved_filename = filename;
    if (!filename.empty() && filename[0] != '/') {
        // Extract directory from current_filename_
        size_t last_slash = current_filename_.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir   = current_filename_.substr(0, last_slash + 1);
            resolved_filename = dir + filename;
        }
    }

    // Read the included file
    std::ifstream file(resolved_filename);
    if (!file.is_open()) {
        reportError("Could not open included file: " + filename + " (resolved to " + resolved_filename + ")",
                    filename_token);
        return;
    }

    // Read the file content
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Create a new namespace for the included file
    std::string include_ns = resolved_filename;  // Use the resolved filename as the namespace

    // Store current scope to restore later
    std::string current_scope = Symbols::SymbolContainer::instance()->currentScopeName();

    // Create and enter the include scope
    Symbols::SymbolContainer::instance()->create(include_ns);
    Symbols::SymbolContainer::instance()->enter(include_ns);

    // Add the file content to the lexer
    auto lexer = std::make_shared<Lexer::Lexer>();
    lexer->setKeyWords(keywords);
    lexer->addNamespaceInput(include_ns, content);

    // Tokenize the included file
    auto tokens = lexer->tokenizeNamespace(include_ns);

    // Parse the included file
    parseScript(tokens, content, resolved_filename);

    // Get all symbols from the included file
    auto included_symbols = Symbols::SymbolContainer::instance()->getAll(include_ns);

    // Get all operations from the included file
    auto included_operations = Operations::Container::instance()->getAll(include_ns);

    // Move all operations from include scope to current scope
    for (const auto & operation : included_operations) {
        Operations::Container::instance()->add(current_scope, std::move(*operation));
    }

    // Clear operations from the include scope
    Operations::Container::instance()->clear(include_ns);

    // Directly copy all symbols from the included file to the current scope
    for (const auto & symbol : included_symbols) {
        try {
            // Determine the appropriate namespace for this symbol based on its kind
            std::string targetNamespace;
            switch (symbol->getKind()) {
                case Symbols::Kind::Variable:
                    targetNamespace = Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE;
                    break;
                case Symbols::Kind::Function:
                    targetNamespace = Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE;
                    break;
                case Symbols::Kind::Constant:
                    targetNamespace = Symbols::SymbolContainer::DEFAULT_CONSTANTS_SCOPE;
                    break;
                default:
                    targetNamespace = Symbols::SymbolContainer::DEFAULT_OTHERS_SCOPE;
                    break;
            }

            // Get the target scope's symbol table
            auto targetTable = Symbols::SymbolContainer::instance()->getScopeTable(current_scope);
            if (targetTable) {
                // Define the symbol directly in the appropriate namespace of the target scope
                targetTable->define(targetNamespace, symbol);
            }
        } catch (const std::exception & e) {
            // Skip if symbol already exists or other error occurs
        }
    }

    // Exit include scope and return to current scope
    Symbols::SymbolContainer::instance()->enterPreviousScope();
}

// Helper to add a 'this' variable to a method scope
void Parser::addThisToMethodScope(const std::string & methodScope) {
    // Extract the class name from the method scope (format: file::className::methodName)
    std::string className;
    size_t      lastColonPos = methodScope.rfind("::");
    if (lastColonPos != std::string::npos) {
        size_t prevColonPos = methodScope.rfind("::", lastColonPos - 1);
        if (prevColonPos != std::string::npos) {
            className = methodScope.substr(prevColonPos + 2, lastColonPos - prevColonPos - 2);
        }
    }

    if (className.empty()) {
        return;
    }

    // Create an empty object with class metadata to serve as 'this'
    Symbols::Value::ObjectMap thisObj;
    thisObj["__class__"] = Symbols::Value(className);

    // Create a 'this' variable with the empty object
    auto thisSymbol = Symbols::SymbolFactory::createVariable("this", Symbols::Value(thisObj), methodScope,
                                                             Symbols::Variables::Type::OBJECT);

    try {
        // Add the 'this' variable to the method scope
        Symbols::SymbolContainer::instance()->defineInScope(methodScope, thisSymbol);
    } catch (const std::exception & e) {
        // Failed to add 'this' to method scope
    }
}
}  // namespace Parser
