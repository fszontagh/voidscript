#include "Parser/Parser.hpp"
#include "utils.h"
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
#include "Lexer/Lexer.hpp"
#include "Lexer/Operators.hpp"
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
                return parseAssignmentStatementNode();  // Parse and return the assignment node
            }
            // If it started with var/this but wasn't postfix or standard assignment,
            // it will fall through to the expression statement parsing below.
        }
    }

    // <<< ADD VARIABLE DEFINITION CHECK HERE >>>
    // Check if current token is a known type keyword OR a registered class identifier
    bool is_type_keyword = (Parser::variable_types.find(currentToken().type) != Parser::variable_types.end());
    bool is_class_name   = (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
                          Modules::UnifiedModuleManager::instance().hasClass(currentToken().value));

    if ((is_type_keyword || is_class_name) &&
        (peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
         peekToken().type == Lexer::Tokens::Type::IDENTIFIER) &&
        peekToken(2).type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT) {
        // Parse as variable definition using the new node-returning function
        return parseVariableDefinitionNode();
    }
    // <<< END VARIABLE DEFINITION CHECK >>>

    // Function call (identifier followed directly by '(')
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER &&
        peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "(") {
        // Parse as a function call statement and return the node
        return parseCallStatement();
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
            Modules::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue) };
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
            Modules::ClassInfo::PropertyInfo info{ propName, propType, std::move(defaultValue) };
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
    // REMOVE Add to operations container
    /*
    Operations::Container::instance()->add(
        Symbols::SymbolContainer::instance()->currentScopeName(),
        Operations::Operation{ Operations::Type::FunctionCall, func_name, std::move(stmt) });
    */
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
                std::string          propName  = propToken.value;
                // Remove $ prefix from property name
                if (!propName.empty() && propName[0] == '$') {
                    propName = propName.substr(1);
                }
                // Create an identifier for the property without $ and push to queue
                output_queue.push_back(ParsedExpression::makeVariable(propName));
                expect_unary = false;
                atStart      = false;
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

            while (!operator_stack.empty()) {
                const std::string & top = operator_stack.top();
                // Stop processing stack if we hit '(' or an operator with lower precedence
                if (top == "(" ||
                    (!Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) >= Lexer::getPrecedence(top)) ||
                    (Lexer::isLeftAssociative(op) && Lexer::getPrecedence(op) > Lexer::getPrecedence(top))) {
                    break;
                }

                // Pop the operator from the stack and apply it
                operator_stack.pop();

                // Apply the operator `top` that was just popped
                if (top == "u-" || top == "u+" || top == "u!" || top == "u++" || top == "u--" || top == "p++" ||
                    top == "p--") {  // Unary ops
                    if (output_queue.empty()) {
                        // Pass the original token for better error location
                        Parser::reportError("Missing operand for unary operator '" + top + "'", token);
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(top, std::move(rhs)));  // Apply 'top'
                } else {
                    // Binary operators
                    if (output_queue.size() < 2) {
                        Parser::reportError("Missing operands for binary operator '" + top + "'", token);
                    }
                    auto rhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    auto lhs = std::move(output_queue.back());
                    output_queue.pop_back();
                    output_queue.push_back(Lexer::applyOperator(top, std::move(rhs), std::move(lhs)));  // Apply 'top'
                }
            }

            operator_stack.push(op);
            consumeToken();
            // After an operator, we expect a value (operand) or a prefix unary operator
            expect_unary = true;
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
        if (Modules::UnifiedModuleManager::instance().hasClass(typeName)) {
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
    auto memberExpr = ParsedExpression::makeMember(std::move(thisExpr), propName, this->current_filename_,
                                                   thisTok.line_number, thisTok.column_number);

    return memberExpr;
}

// Parse an assignment statement (variable, object member, 'this' member) and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseAssignmentStatementNode() {
    Lexer::Tokens::Token baseToken;
    std::string          baseName;
    bool                 isThisAssignment = false;

    // Determine base: 'this' or variable identifier
    if (currentToken().type == Lexer::Tokens::Type::IDENTIFIER && currentToken().value == "this") {
        baseToken        = consumeToken();  // Consume 'this'
        baseName         = "this";
        isThisAssignment = true;
    } else if (currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER) {
        baseToken = consumeToken();  // Consume variable identifier
        baseName  = parseIdentifierName(baseToken);
    } else {
        // This case should ideally not be reached if called correctly after checks
        // But it's useful as a safety check
        reportError("Expected variable name or 'this' at start of assignment", currentToken());
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
            lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>("this");
        } else {
            lhsNode = std::make_unique<Interpreter::IdentifierExpressionNode>(baseName);
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
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_INCLUDE) {
        parseIncludeStatement();
    } else if (token_type == Lexer::Tokens::Type::KEYWORD_CONST) {
        parseConstVariableDefinition();
    }
    // Variable definition (type keyword or known class name followed by variable identifier)
    else if ((Parser::variable_types.find(token_type) != Parser::variable_types.end() ||
              (token_type == Lexer::Tokens::Type::IDENTIFIER &&
               Modules::UnifiedModuleManager::instance().hasClass(token_val)))) {
        // Check if it's an array declaration: Type[] $var = ...
        bool is_array_decl = (peekToken().type == Lexer::Tokens::Type::PUNCTUATION && peekToken().value == "[" &&
                              peekToken(2).type == Lexer::Tokens::Type::PUNCTUATION && peekToken(2).value == "]" &&
                              (peekToken(3).type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                               peekToken(3).type == Lexer::Tokens::Type::IDENTIFIER) &&
                              peekToken(4).type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

        // Check if it's a simple variable declaration: Type $var = ...
        bool is_simple_decl = ((peekToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||
                                peekToken().type == Lexer::Tokens::Type::IDENTIFIER) &&
                               peekToken(2).type == Lexer::Tokens::Type::OPERATOR_ASSIGNMENT);

        if (is_array_decl || is_simple_decl) {
            parseVariableDefinition();  // This function should handle both cases now (since parseType consumes [])
        }
        // If it starts with a type but isn't a declaration, it might be an expression statement (like a class method call)
        else {
            // Fall through to expression statement parsing
            auto stmtNode = parseStatementNode();
            if (stmtNode) {
                Operations::Container::instance()->add(
                    Symbols::SymbolContainer::instance()->currentScopeName(),
                    Operations::Operation{ Operations::Type::Expression, "", std::move(stmtNode) });
            } else {
                reportError("Invalid statement starting with type keyword", currentTok);
            }
        }
    }
    // Prefix increment/decrement statement (++$var; or --$var;)
    else if (token_type == Lexer::Tokens::Type::OPERATOR_INCREMENT &&
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
                // If not postfix and not assignment, assume it's an expression statement (like a method call)
                auto stmtNode = parseStatementNode();  // parseStatementNode handles expression statements
                if (stmtNode) {
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

// NEW: Parse a variable definition and return its node
std::unique_ptr<Interpreter::StatementNode> Parser::parseVariableDefinitionNode() {
    Symbols::Variables::Type var_type = this->parseType();  // Add this->

    // Variable name
    Lexer::Tokens::Token id_token;
    if (this->currentToken().type == Lexer::Tokens::Type::VARIABLE_IDENTIFIER ||      // Add this->
        this->currentToken().type == Lexer::Tokens::Type::IDENTIFIER) {               // Add this->
        id_token = this->consumeToken();                                              // Add this->
    } else {
        this->reportError("Expected variable name", this->currentToken());            // Add this->
        return nullptr;                                                               // Should not be reached
    }
    std::string var_name = this->parseIdentifierName(id_token);                       // Add this->
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
}  // namespace Parser
