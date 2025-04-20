 # Step 2: AST Nodes & Interpreter Scaffolding

- Create AST node classes:
  - `ClassDefinitionStatementNode` in `src/Interpreter/ClassDefinitionStatementNode.hpp`:
    - Store class name, lists of private fields, public fields, and methods (use placeholder containers).
    - Implement `interpret(Interpreter &)` as a stub: register the class definition in a new `ClassRegistry`.
  - `NewExpressionNode` in `src/Interpreter/NewExpressionNode.hpp`:
    - Store target class name and constructor argument expression pointers.
    - Implement `evaluate(Interpreter &)` as a stub: lookup the class in `ClassRegistry` and return an empty `Symbols::Value(ObjectMap{})`.
- Introduce `ClassRegistry` singleton in `src/Interpreter/ClassRegistry.hpp`:
  - Maintain a map from class names to their definition data.
  - Methods to register a new class and lookup existing ones.
- Wire up the interpreter:
  - In `OperationsFactory` or the statement execution loop, when a `ClassDefinitionStatementNode` is interpreted, call `ClassRegistry::registerClass(...)`.
  - Ensure `NewExpressionNode::evaluate` uses `ClassRegistry::getClass(...)` and throws an error if undefined.
- Leverage existing `Symbols::Value::ObjectMap` to hold instance fields temporarily.
  - No privacy enforcement yet.
- Add integration tests:
  - Parse and evaluate a script:
    ```
    class test1 { public: string $x; }
    test1 $obj = new test1("hello");
    printnl(typeof($obj));
    ```
  - Verify no errors and that `$obj` is treated as an object.