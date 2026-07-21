# VoidScript - notes for working on the interpreter

C++20 tree-walking interpreter for a statically-typed, `$`-sigil scripting language.
`src/Lexer` → `src/Parser` (shunting-yard, builds `ParsedExpression`) →
`src/Interpreter` (statement/expression nodes) with modules under `src/Modules/BuiltIn`
and dynamic ones under `Modules/`.

## Build and test

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)          # ~2 min; run it in the background
ctest --test-dir build                  # 102 tests
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

All 62 pass, and every script is also a ctest case (globbed, so new ones are picked up
automatically). ctest redirects stdin from /dev/null - it otherwise hands the tests the
terminal and the interactive scripts block forever. **Compare the
pass *set*, not the count** - it is easy to fix one script and break another and see no
change in the total.

**Do not use short timeouts.** This is a Debug build and slow to start; `timeout 3`
under load produces spurious "hangs" that look exactly like parser infinite loops.

## Two hazards that have each caused real bugs

**1. `ValuePtr` has no implicit conversion to bool - and must not regain one.**

It used to. `operator bool()` returned the *value's* truthiness - `0`, `""`, `false` and
empty objects were all falsy - and *threw* on a genuinely null value, so `if (!value)`
was wrong in both directions while looking exactly like a null check.

It made `switch (0)` and `switch ("")` report "must evaluate to a non-null value", broke
every enum whose first member had the implicit value 0, made `json_encode(0)` throw,
made `var_dump(0)` print `NULL`, and made every `CurlClient` request die on a missing
options argument.

The conversion is gone, so `if (!value)` no longer compiles. Use `value->is_null()` to
test presence and `value.toBool()` to test truthiness. Do not re-add bool to the
universal conversion operator's `requires` clause.

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

- **`-Werror=switch` is on.** Switches over an `enum class` have no `default:` arm on
  purpose, so adding an enumerator fails the build until every switch handles it. A
  `default: throw` once hid a missing case and broke the entire `switch` statement at
  runtime.
- **Control-flow exceptions derive from `ControlFlowSignal`, not `std::exception`.**
  Break, Continue and Return cannot be caught by a generic handler, by construction. Do
  not "helpfully" give them a `std::exception` base.
- **`REGISTER_FUNCTION`/`REGISTER_METHOD` are macros.** A comma anywhere at the top
  level of an argument splits it - that rules out two-item lambda captures like
  `[this, helper]` and multi-variable declarations inside a lambda body. The error is a
  confusing "passed 6 arguments, but takes just 5".
- **The parse loop needs a progress guard.** `Parser::parse` checks that
  `parseTopLevelStatement()` consumed at least one token, otherwise a branch that falls
  through without consuming spins forever. Keep it when adding statement kinds.
- **Native modules must store per-instance state ON the object, not in a `static`
  map keyed by `args[0].toString()`.** A fresh class instance serialises to just its
  `$class_name`, so every instance produces the same key and they all collide onto one
  entry - two `new Imagick()`s ended up sharing one image, and DateTime one timestamp.
  Stamp a handle into the object's map (e.g. `__image_id__`) and read it back; see
  ImagickModule / DateTimeModule.
- **`new` returns the object it built, not the constructor's return value.** A native
  constructor mutates `args[0]` in place (stamp state there) and its return is ignored.
- **Method-call nesting is bounded at depth 100** by a `DepthGuard` RAII object in
  `MethodCallExpressionNode`. It must stay RAII: an earlier manual counter leaked on
  some return paths and counted cumulative calls, capping whole programs at ~100 method
  calls.
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
