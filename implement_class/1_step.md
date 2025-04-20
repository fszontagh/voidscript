 # Step 1: Lexer & Parser Support

- Add new keywords to the lexer:
  - Recognize `class`, `private`, `public`, and `new` as keywords rather than identifiers.
  - Update the keyword map in `Parser::keywords` (and any relevant lexer keyword lists) to include these words.
  - Map these keywords to appropriate token types in `Lexer::Tokens::Type` (e.g., KEYWORD_CLASS, KEYWORD_PRIVATE, KEYWORD_PUBLIC, KEYWORD_NEW).
- Extend the parser (`Parser.cpp`/`Parser.hpp`):
  - Add a new method `parseClassDefinition()`:
    - Syntax: `class <Identifier> { (<accessSpecifier>: <memberDeclarations>)* }`.
    - Inside the body, accept repeated `private:` and `public:` sections with field/method declarations.
    - For now, consume tokens until the matching `}` without constructing full AST nodes (stub behavior).
  - In `parseStatement()`, detect the `class` keyword and invoke `parseClassDefinition()`.
  - Support `new` expressions in `parseParsedExpression()` or the shunting-yard parser:
    - When encountering the `new` keyword, parse `new <ClassName>(<argList>)` into a placeholder `NewExpressionNode`.
  - Add stub AST node declarations so compilation succeeds:
    - `ClassDefinitionStatementNode : public StatementNode` in `src/Interpreter/` (header only).
    - `NewExpressionNode : public ExpressionNode` in `src/Interpreter/` (header only).
- Add simple tests to verify that parsing a script containing `class test1 {}` and `new test1()` does not produce lexer or parser errors.