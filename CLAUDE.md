# VoidScript - notes for working on the interpreter

C++20 tree-walking interpreter for a statically-typed, `$`-sigil scripting language.
`src/Lexer` → `src/Parser` (shunting-yard, builds `ParsedExpression`) →
`src/Interpreter` (statement/expression nodes) with modules under `src/Modules/BuiltIn`
and dynamic ones under `Modules/`.

## Build and test

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)          # ~2 min; run it in the background
ctest --test-dir build                  # 35 tests
ctest --test-dir build -R Regression    # the bug regression suite only
```

Every fixed bug has a runnable repro in `test_scripts/regression/`, wired into ctest via
`CMakeLists.txt`. Add one for any fix. `BUGS.md` records the root cause of each.

Sweeping the whole script suite:

```bash
for f in test_scripts/*.vs; do
  timeout 15 ./build/voidscript "$PWD/$f" </dev/null >/dev/null 2>&1; echo "$? $f"
done
```

55 of 62 pass. The other seven need an interactive TTY (readline) or command-line
arguments and an image (image_resize), so they cannot pass unattended. **Compare the
pass *set*, not the count** - it is easy to fix one script and break another and see no
change in the total.

**Do not use short timeouts.** This is a Debug build and slow to start; `timeout 3`
under load produces spurious "hangs" that look exactly like parser infinite loops.

## Two hazards that have each caused real bugs

**1. `if (!somePtr)` on a `ValuePtr` does not test the pointer.**

`ValuePtr::operator bool()` (`src/Symbols/Value.hpp`) returns the *value's* truthiness -
`0`, `""`, `false` and empty objects are all falsy - and it *throws* on a genuinely null
value instead of returning false. So the idiom is wrong in both directions.

It made `switch (0)` and `switch ("")` report "must evaluate to a non-null value", broke
every enum whose first member had the implicit value 0, made `json_encode(0)` throw
"Cannot convert null ValuePtr to JSON", and made `var_dump(0)` print `NULL`.

Use `value->is_null()`. `operator->` materialises a NULL `Value` when the pointer is
empty, so it is safe on its own and needs no `!value` guard in front of it.

**2. Function bodies are re-parsed from a bare token slice with no `END_OF_FILE`
token.**

`Parser::parseBlockInNewScope` (`src/Parser/Parser.cpp`) slices the body tokens and runs
them through a fresh `Parser`. That slice has no terminator token, so any `currentToken()`
peek at the end of a construct throws "Cannot access token at end of stream" - but *only*
when that construct happens to be the last statement of a function, which makes it look
like a bug in the construct itself.

Guard every trailing peek with `isAtEnd()`. This bit an optional-semicolon check after
`switch`, and was reported as "return inside a switch case fails to parse" when `return`
had nothing to do with it.

## Other things worth knowing

- **The parse loop needs a progress guard.** `Parser::parse` checks that
  `parseTopLevelStatement()` consumed at least one token, otherwise a branch that falls
  through without consuming spins forever. Keep it when adding statement kinds.
- **Control-flow exceptions are not errors.** `ReturnException` does not derive from
  `std::exception`; `BreakException` derives from `std::runtime_error` and therefore
  *will* be caught by a generic `catch (const std::exception&)`. Any new handler must
  rethrow both, plus `ThrowException`, before its generic arm - see
  `TryStatementNode::interpret`.
- **Arrays are `ObjectMap`s keyed by the decimal index** (`"0"`, `"1"`, ...), so an
  element write is the same operation as an object property write.
- **A declaration's type constrains what the expression produces, not what appears
  inside it.** `pushOperand` used to reject literals whose type differed from the
  declared one, which broke `string $s = "x" + 5;` and `auto $b = true;`.
  `DeclareVariableStatementNode` does the real check.
- **`Interpreter::ExpressionNode::line`/`column` default to 0**, and callers treat 0 as
  "fall back to the enclosing statement". They were once uninitialised, which printed
  stack garbage as the error location.

## Language surface

`~/.claude/skills/voidscript/SKILL.md` documents the language for *writing* scripts and
is kept in sync with the build. If you change language behaviour, update it - another
agent may be using it right now. Verify any snippet you put in it by running it.
