# Roadmap: Fixing Class Method Invocation Bug

This document outlines a step-by-step plan to resolve the issue where class method bodies are not executed, causing method calls to have no effect.

## 1. Investigate and Confirm Root Cause [DONE]
- The parser's `parseFunctionBody` locates the opening brace by value (`std::find`), matching the class's brace instead of the method's.
- Method bodies are consequently not tokenized or recorded under the correct namespace.

## 2. Refactor Parser: `parseFunctionBody` [DONE]
1. Change signature to accept the opening-brace token **index** instead of the `Token` object.
2. Capture the method body by slicing `tokens_` using the known starting index and matching closing brace index.
3. Adjust `current_token_index_` advancement to follow the parsed method body correctly.
4. Ensure the new namespace is created and entered before parsing the body, and exited afterwards.

## 3. Adjust `OperationsFactory` and Namespace Registration [DONE]
- Verify that `FuncDeclaration` operations are recorded under `ClassNamespace` before any method invocations.
- Remove any temporary hacks that replay `FuncDeclaration` operations at runtime.

## 4. Implement Method Invocation [DONE]
- In `MethodCallExpressionNode`, retrieve and enter the correct method namespace.
- Bind `this` and parameters into `SymbolContainer`.
- Execute all operations recorded for that namespace, handling `ReturnException` to return values.

## 5. Testing [DONE]
1. Extend `test_scripts/class_test.vs` with:
   - Methods containing side effects (`printnl`). [DONE]
   - Methods returning values (`incrementA`). [DONE]
2. Run:
   ```bash
   cmake --build build
   ./build/voidscript test_scripts/class_test.vs
   ```
3. Observed output:
   ```
   $f->a = 10
   $f->b = 2
   $f->b + $f->a = 12
   object
   ```
   Execution stopped after `typeof($f)`: no output from `test()` or `incrementA()`.
4. Observation: method calls (`$f->test()`, `$f->incrementA()`) did not execute and produced no output or value; no errors thrown.
5. Next: debug method call handling in parser and interpreter.

## 6. Debug Property Access Parsing [DONE]
- Refined `ExpressionBuilder` to accept variable identifiers and string literals as property names without strict kind checks.
- Simplified extraction of `propName` to use `expr->rhs->name` or literal value where appropriate.
- Verified parsing of `$f->a`, `$f->b`, `this->a`, etc., now succeeds.
- Added a unit test for object literal property access via arrow syntax:
  ```vs
  printnl(({"x": 42})->x);  // expect 42
  ```

## 7. Retest and Validate Method Invocation [DONE]
1. Reran `class_test.vs`:
   - `$f->test()` outputs "Test method called".
   - `$f->incrementA()` returned 11; `$f->a` updated to 11.
2. Verified object-literal property access test:
   ```vs
   printnl(({ "x": 42 })->x);  // expect 42
   ```
   Output: `42`.

## 8. Cleanup and Documentation [DONE]
- Remove debug logging and temporary workarounds.
- Update repository documentation (e.g., CONTRIBUTING.md) to reflect parser change patterns.

Implementing this roadmap fixes the method-body registration at the root cause and restores correct class-method behavior.