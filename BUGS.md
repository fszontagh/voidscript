# VoidScript - Known Bugs and Gaps

Status of the language as verified against a Debug build of `build/voidscript`.
Every entry was reproduced by running the interpreter, not inferred from reading source.
Minimal repros are given so each one can be re-checked after a change.

Regression tests live in `test_scripts/regression/` and are wired into ctest:

```bash
cmake -S . -B build -DBUILD_TESTS=ON && ctest --test-dir build -R Regression
```

Note the interpreter is a Debug build and slow to start. When sweeping the script suite,
use a generous timeout - short ones produce spurious "hangs" under load.

---

## Fixed

### 1. Parser hung forever on a declaration missing the `$` sigil

```voidscript
int y = 5;      // no $ sigil
```

Hung forever: no output, no error, 100% CPU, needed SIGKILL. Because parsing completes
before execution, even `print()` calls on earlier lines never appeared.

**Cause:** `Parser::parse()` looped on `parseTopLevelStatement()`, which has a branch
matching a type keyword that only proceeds if the *next* token is a
`VARIABLE_IDENTIFIER`. Without the sigil that token is a plain `IDENTIFIER`, so the
branch fell through and the function returned **without consuming a token** - the loop
never advanced.

**Fix:** a progress guard in the top-level loop (if nothing was consumed, report a
syntax error) as a general safety net against any silent-fallthrough branch, plus a
targeted diagnostic: `Variable names must start with '$' (did you mean '$y'?)`.

The `$` sigil is mandatory by design - `Parser::parseType()` distinguishes variables
from class and enum names by it, so making it optional would make the grammar ambiguous.

This one bug accounted for 6 failing scripts (`math_test.vs`, `direct_test.vs`,
`basic_module_test.vs`, `function_availability_test.vs`, `readline_integration_test.vs`,
`simple_readline_test.vs`), all written in a sigil-less style that was never valid.

Test: `regression/missing_sigil_syntax_error.vs` (a TIMEOUT is the assertion)

### 2. `return` from a call in statement position aborted the process

```voidscript
function f() { printnl("a"); return; }
f();
// a
// terminate called after throwing an instance of 'Interpreter::ReturnException'
```

SIGABRT - a C++ crash, not a VoidScript error. Also hit any value-returning function
called for its side effects.

**Cause:** `ReturnException` does not derive from `std::exception`, and
`CallStatementNode` caught only `Exception` and `std::exception`. `CallExpressionNode`
had the right catch; the statement-position node was missed.

**Fix:** catch `ReturnException` inside the operation loop, mirroring
`CallExpressionNode`. Catching *inside* the loop keeps `enterPreviousScope()` reachable,
so the per-call scope is not leaked on return.

Test: `regression/bare_return.vs`

### 3. `switch` always failed with "Unknown operation type"

**Cause:** the lexer, parser and `SwitchStatementNode` were all present and correct. The
parser emitted `Operations::Type::ControlFlow`, but `Interpreter::runOperation` had no
`case` for it, so every switch fell to `default: throw`.

**Fix:** added the missing `case`. Also dropped the mandatory `;` after the closing `}`,
which no other block statement requires and whose error pointed at the *following*
statement, making the switch body look malformed. A stray `;` is still tolerated.

Test: `regression/switch_statement.vs`

### 4. Indexing did not compose with member access, and could not be assigned to

```voidscript
printnl($q->outputs[0]);   // Invalid member access ... right side has kind: Binary
$a[0] = 9;                 // Unsupported types in binary expression: object and int
```

Two independent causes, both surfacing through `[`.

**Cause A - precedence.** `getPrecedence()` had no entry for `"[]"`, so it fell to the
`-1` default and bound *looser* than every operator including `&&`. `[` is already a
postfix accessor that pops its operand off the output queue, but for `$q->outputs[0]`
the `->` is still pending on the operator stack, so it popped the bare `outputs` and the
result grouped as `$q -> (outputs[0])`.

**Cause B - assignment target.** `$a[0] = 9` built `BinaryExpressionNode(lhs, "=", rhs)`
around an inner `BinaryExpressionNode(a, "[]", 0)`. `BinaryExpressionNode` implements
neither `=` nor `[]`, so this was a node the interpreter could never evaluate. There was
also an explicit "multi-dimensional array assignment not yet supported" bail-out.

**Fix:** give `"[]"` accessor precedence and drain pending accessors before it takes its
operand; and replace the assignment hack with `IndexedAssignmentStatementNode`. Arrays
are `ObjectMap`s keyed by decimal index, so an element write is the same operation as a
property write, and `ArrayAccessExpressionNode::evaluate` returns the shared `ValuePtr`
rather than a clone, so writing through the evaluated container mutates the original.
Multi-dimensional assignment works as a result.

Arrays were never designed to be immutable - this was one missing table entry plus an
unevaluatable node.

Test: `regression/index_operator.vs`

### 5. No bitwise operators

`|`, `&`, `^`, `~`, `<<`, `>>` were absent from the lexer; `|` did not even tokenise
(`Type: UNKNOWN`). Added `&`, `|`, `^`, `~` to the single-character table and `<<`/`>>`
as a new two-character table. The two-character pass runs first, so `&&` and `||` still
win over a lone `&` or `|`.

The precedence table was also replaced with the full C ladder. It previously had five
levels, lumping all six comparison operators together and `&&` with `||`. Now
`1 | 2 + 4` is 7 and `1 << 3 > 4` is true.

Bitwise operators are integer-only by design, and shift amounts outside [0, 32) are
rejected rather than left as C++ undefined behaviour.

This was the last blocker for `enum_access.vs`, which combines flag enums with `|`.

Test: `regression/bitwise_operators.vs`

### 6. `int $a[3];` sized array declaration

Was a syntax error. Now declares N elements initialised to the zero value for the
element type (`0`, `0.0`, `""`, `false`) rather than null, so an untouched element reads
back usable and writing to one is still type checked.

This is what actually broke `enum_advanced.vs`, `enum_comprehensive.vs` and
`enum_switch.vs`; it had been mis-attributed to #4.

Test: `regression/sized_array_decl.vs`

### 10. Numeric types did not coerce

```voidscript
float $f = 2.718;   // was: expected 'float' but got 'double'
double $d = 3;      // was: expected 'double' but got 'int'
```

Every float literal lexes as double, so even the obvious declaration was a type error.
Numeric values now adopt the target numeric type. Narrowing to int is still refused
(`int $i = 3.9;` errors) since it would silently drop the fraction.

The rule lives in one place, `Symbols::tryNumericCoerce`, applied by both the
declaration and assignment paths - which had drifted apart and enforced different rules.

Test: `regression/numeric_conversion.vs`

### 11. Escaped exceptions aborted the process

`VoidScript::run()` caught only `std::exception`, and `ReturnException` does not derive
from it, so a top-level `return` escaped into `std::terminate` (exit 134, bare
"terminate called" with no diagnostic).

A top-level `return` now ends the script cleanly, as in PHP, and a `catch (...)`
backstop reports anything else that escapes instead of aborting.

Test: `regression/toplevel_return.vs`

### 13. Array append `$a[] = 3`

An empty subscript now parses to a null index expression, resolved at runtime to the
current element count. It is rejected anywhere but the last position of an assignment
target. Works through members too: `$bag->items[] = 8;`.

Test: `regression/index_operator.vs`

### 16. `switch` only accepted integers and enums

String dispatch - the most common use for switch in a scripting language - errored with
"Switch expression must evaluate to a non-null integer type". Now accepts STRING as
well, compares by type, and rejects a case whose type does not match the switch
expression rather than silently never matching.

### 17. A `switch` ending a function body failed to parse

```
Cannot access token at end of stream.
```

**This was a regression** from making the trailing `;` after a switch block optional
(#3). Function bodies are re-parsed by a separate `Parser` over a raw token slice that
carries **no `END_OF_FILE` token**, so peeking with `currentToken()` for that optional
semicolon threw when a switch was the last statement of a function. Guarded with
`isAtEnd()`.

Reported as "return inside a switch case fails to parse", but `return` was incidental -
any switch ending a function body failed. Bodies returning from every case just always
hit it.

### 18. `switch` rejected falsy values (`0` and `""`)

The guard read `!switch_value || switch_value->is_null()`, but
`ValuePtr::operator bool()` returns the **value's truthiness**, not pointer validity.
So `switch (0)` and `switch ("")` were both reported as null, and any enum member with
an implicit value of 0 was unswitchable - the real cause of `enum_switch.vs` failing.

Dropped the truthiness test; `operator->` materialises a NULL `Value` when empty, so
`is_null()` is safe alone.

**General hazard:** `if (!somePtr)` on a `ValuePtr` does not mean what it looks like.
Other call sites doing this are likely to have the same latent bug.

### 9. String + number concatenation

`"count=" + $n` threw "Unsupported types in binary expression: string and int", so the
most natural way to build a message was an error. `+` already concatenates two strings,
so it now also concatenates when exactly one side is a string.

Limited to `+` on purpose: comparing a string with a number stays a type error, and
numeric `+` is untouched. The test pins `2 + 3 == 5` so this cannot drift into JS-style
`"2" + 3 == "23"`.

Test: `regression/string_concat.vs`

### 14. Ternary conditional `?:`

`?` was not lexed at all. Added `cond ? a : b`, parsed after the shunting-yard rather
than inside it - the ternary is the loosest-binding construct and right associative, so
recursing on the else branch gives both for free and `a ? b : c ? d : e` groups as
`a ? b : (c ? d : e)`. Only the taken branch is evaluated.

Its absence also caused misleading errors elsewhere: the parser guessed at a ternary on
any unexpected token, which is why `try {` reported "Expected token PUNCTUATION with
value ':'".

Test: `regression/ternary.vs`

### 15. Garbage source locations on runtime errors

`Interpreter::ExpressionNode` declared `int line; size_t column;` with **no
initialiser**, so any node that did not set them carried stack garbage into the error
formatter - hence `at line: 1942890312`, different on every run.

Zero is the sentinel every caller already tests before falling back to the enclosing
statement's location, so default-initialising to zero makes those fallbacks work as
designed. Errors now report a real, stable location.

Test: `regression/error_source_location.vs`

### 19. `json_encode` and `var_dump` mishandled falsy values

The same `ValuePtr` truthiness trap as #18, at four more sites:

- `valueToJson`, `valueToJsonWithContext`, `canConvertToJson` - `json_encode(0)`,
  `json_encode("")` and `json_encode(false)` all failed with "Cannot convert null
  ValuePtr to JSON". The guards were redundant as well as wrong (the switch below
  already maps `NULL_TYPE` to JSON `null`), so they were removed.
- `var_dump_recursive` - printed `NULL` for `0`, `""` and `false`. Now tests
  `is_null()`.

A sweep found the remaining `if (!x)` hits were `std::optional`, which is correct.

Test: `regression/falsy_values.vs`

### 20. Literals were gated on the declared type

`pushOperand()` rejected a literal whose type did not match the declaration's type
*anywhere* in the expression, so `string $s = $c ? "a" : "b";` failed on the condition's
numeric literal, and `string $s = "x" + 5;` failed too.

A declaration's type says what the whole expression must **produce**, not what may
appear inside it. The gate is gone; `DeclareVariableStatementNode` still type checks the
resulting value, and its message ("expected 'string' but got 'int'") is clearer than the
parse error it replaced.

Test: `regression/string_concat.vs`

---

### 7. `try` / `catch` / `throw`

There was no error recovery of any kind: every runtime error killed the process.
`try {` also misparsed into "Expected token PUNCTUATION with value ':'" because the
parser guessed at a ternary on any unexpected token.

```voidscript
try { ... } catch { ... }
try { ... } catch (string $e) { ... }
throw "message";
```

The catch variable is optional, matching the form the existing scripts were written
against. Built-in runtime errors are catchable too, not just script throws.

Control flow is deliberately NOT catchable, and this needed care: `BreakException`
derives from `std::runtime_error`, so a plain `catch (const std::exception&)` would
swallow a `break` inside a try and make the enclosing loop infinite. It is rethrown
explicitly ahead of the generic handler; `ReturnException` likewise. The test pins both.

Four statement nodes wrapped any `std::exception` into an `Interpreter::Exception`,
flattening a script throw into a message string before it could reach its catch; they
now rethrow `ThrowException` untouched.

Test: `regression/try_catch.vs`

### 8. String interpolation

```voidscript
printnl("Hello $name");          // bare identifier
printnl("${name}s");             // braced, to delimit the name
printnl("${obj->field}");        // braced member access, any depth
printnl("costs \$5");            // escaped, stays literal
printnl('raw $name');            // single quotes never interpolate
```

Works from the token's raw lexeme, not its processed value: once the lexer resolves
escapes, an escaped `\$` is indistinguishable from a real one, and that distinction is
what decides whether to interpolate. The lexeme keeps the quotes too, which is how
single vs double quoting is detected without adding a token field.

Test: `regression/string_interpolation.vs`

### 21. Only decimal numeric literals

`0xFF`, `0b1010`, `0o17`, `1e3` and `1_000_000` were all syntax errors - which blocked
`enum_comprehensive.vs`, since it declares colours as `0xFF0000`.

Enum values were separately read with `std::stoi`, which silently returns 0 for
`"0xFF0000"` (it parses the leading `0` and stops), so every hex enum value was zero.
Both integer-literal sites now use a base-aware helper.

Test: `regression/numeric_literals.vs`

### 22. A for loop's induction variable leaked into the enclosing scope

The initialiser ran in the enclosing scope, so a second `for (int $i = ...)` anywhere in
the same scope failed with "Variable 'i' already declared". The loop already builds a
scope keyed by source position, so entering it before running the initialiser was
enough.

**Behaviour change:** a declared induction variable is no longer visible after the loop,
matching C. Since that removed the only way to observe the counter afterwards, the type
in a C-style for-init is now optional:

```voidscript
for (int $i = 0; ...)    // declares $i, scoped to the loop
for ($g = 0; ...)        // assigns to an existing $g, which outlives the loop
```

Test: `regression/loop_scope.vs`

### 23. `auto` was only legal as a for-each value type

`auto $modules = list_modules();` failed with "expected 'auto' but got 'object'". It now
adopts the initialiser's type, as in C++, resolved before every check so the symbol
carries the real type. An `auto` with no initialiser is rejected.

Test: `regression/auto_type.vs`

### 24. `new` never called native constructors

`NewExpressionNode` only executed constructor operations from the Operations container -
script-defined constructors. A module-supplied constructor is a native callable with no
such operations, so `new DateTime()` found the constructor, ran nothing, and returned an
object carrying only its `$class_name`. This affected any module class, not just
DateTime.

The DateTime module also stored its timestamp two incompatible ways: `__construct` and
`year()`/`hour()`/`minute()`/`second()` used a static map keyed by `args[0].toString()` -
the object's serialised *contents*, which is not identity - while `day()` and `month()`
read a `__timestamp__` field nothing ever wrote. Everything now uses `__timestamp__` on
the object.

Test: `regression/datetime_object.vs`

### 25. Only `++`/`--` allowed as a for-loop increment

Ruled out every step size other than one. Any assignment now works, including compound
forms: `for (int $i = 0; $i < 6; $i += 3)`.

Test: `regression/loop_scope.vs`

### 26. `if (!ptr)` on a ValuePtr was legal, and always wrong

`ValuePtr` had an implicit conversion to bool that returned the *value's*
truthiness - so `0`, `""` and `false` read as absent - and threw outright on a
genuinely null value. The idiom was wrong in both directions and had already caused
#18 and #19.

The conversion is gone. `toBool()` asks for truthiness; `is_null()` asks whether a
value is present. Removing it made the compiler find the remaining 21 sites, five of
which were null checks in disguise: the interpreter's this-object access check (a class
instance with an empty member map read as absent, silently skipping a private-access
check), `SymbolContainer::dumpValue` and `CompilerBackend::valuePtrToCCode` (both
rendered `0`, `""` and `false` as nothing), and nine XmlModule handler lookups that
threw instead of reporting a missing handler.

### 27. `default:` arms hid missing enum cases

`Interpreter::runOperation` had `default: throw Exception("Unknown operation type")`,
which swallowed a missing `Operations::Type::ControlFlow` case and broke every `switch`
statement in the language at runtime rather than at build time (#3).

The arm is gone and `-Wswitch -Werror=switch` is on project-wide, so adding a case to
any enum now fails the build until every switch handles it. Verified the guard fires.

### 28. Control-flow exceptions could be swallowed by any generic handler

`BreakException` derived from `std::runtime_error`, so a plain
`catch (const std::exception &)` absorbed a `break` - in `TryStatementNode` that turned
the enclosing loop infinite.

`Break`, `Continue` and `Return` now share a `ControlFlowSignal` base that deliberately
does **not** derive from `std::exception`, so a new catch site cannot get this wrong by
omission. Correspondingly, the eight sites that rethrew `Exception` before wrapping now
rethrow `BaseException`, which `ThrowException` also derives from, so a script `throw`
reaches its catch intact without every handler needing a special case.

### 29. `continue` did not exist

Not a keyword, not parsed: `continue;` failed with "Identifier 'continue' not found".
Implemented for every loop kind, including inside a `try`. In a C-style `for` it falls
through to the increment, since skipping that would spin the loop forever.

`break` and `continue` outside any loop are now parse-time errors naming the problem,
rather than escaping to the top-level catch-all as "Internal error: unhandled
exception".

Test: `regression/break_continue.vs`

### 30. The CurlClient was entirely unusable, and three verbs were unregistered

Four separate defects: `parseOptions` used `if (!options || ...)` and so threw on a
missing options argument (#26); `mergeOptions` returned the map tagged as `CLASS` while
`parseOptions` required `OBJECT`, so every request rejected the options it had just
built; `createResponse` wrote the class marker as `__class__` but the interpreter reads
`$class_name`, so every `CurlResponse` method failed; and `curlPost`, `curlPut` and
`curlDelete` were implemented but never registered, leaving GET the only reachable verb.

Test: `regression/curl_client.vs` (uses a `file://` URL, so no network).

### 31. Missing standard library: base64, sleep, and most string operations

`base64` and `sleep` were absent from the whole tree. The String module had only
`string_length`, `string_replace` and `string_substr`, so the documented way to test for
a substring was to replace it and compare lengths.

Added `base64_encode`/`base64_decode` (verified byte-for-byte against the system
`base64`), `sleep`/`usleep`, and `string_to_upper`, `string_to_lower`, `string_trim`,
`string_reverse`, `string_index_of`, `string_contains`, `string_starts_with`,
`string_ends_with`, `string_repeat`, `string_split`, `string_join`.

Tests: `regression/base64_sleep.vs`, `regression/string_functions.vs`

### 32. Imagick had no pixel access; optional native parameters did not work

Added `getPixel(x, y)` and `setPixel(x, y, r, g, b [, a])`, channels 0-255, scaled
rather than assuming Quantum depth. `setPixel` calls `modifyImage()` first so the write
is not lost to copy-on-write sharing, and enables the alpha channel when a non-opaque
alpha is written to an image that has none.

This exposed a framework gap: `FunctionParameterInfo` has an `optional` flag, but the
native-method arity check compared against the total count and ignored it, so a method
could declare an optional parameter and never be callable without it. Fixed for every
native method.

Test: `regression/imagick_pixels.vs`

### 33. Binary data was never actually unsafe

The original report rated binary handling "WEAK/MISSING" because file I/O is
`std::string`-based. That was wrong: `std::string` holds NUL bytes fine. 256 random
bytes and a real PNG both round-trip through `file_get_contents`/`file_put_contents`
byte-identical, and through `base64_encode`/`base64_decode` as well.

---

## Open


Nothing known. Every defect found is fixed and covered by a regression test in
`test_scripts/regression/`; ctest runs 102 tests including every script in
`test_scripts/`.

Two things are worth remembering when working on the interpreter:

- **`if (!somePtr)` on a `ValuePtr` no longer compiles** - the implicit conversion was
  removed (#26). Use `->is_null()` to test presence, `toBool()` to test truthiness.
- **Function bodies are re-parsed from a bare token slice with no `END_OF_FILE`
  token.** Any `currentToken()` peek at the end of a construct must be guarded with
  `isAtEnd()`, or it throws only when that construct ends a function. It caused #17.

---

## Module gaps

Verified by grepping `src/` and `Modules/` and by running each capability.

| Capability | Status |
|---|---|
| HTTP GET/POST/PUT/DELETE, headers, JSON body | available - `curlGet`/`curlPost`/`curlPut`/`curlDelete`, and the OOP `CurlClient`/`CurlResponse` |
| JSON | available - `json_decode`, `json_encode` |
| Text and **binary** file I/O | available - byte-identical round trips, NUL bytes included |
| base64 | available - `base64_encode`, `base64_decode` |
| sleep | available - `sleep`, `usleep` |
| String operations | available - case, trim, split, join, index_of, contains, starts/ends_with, repeat, reverse, substr, replace, length |
| Process execution | available - `process_run`, `process_check`, `process_spawn` (no shell; argv array) |
| Image manipulation | available - read/write/crop/resize/blur/rotate/flip plus `getPixel`/`setPixel` with alpha |
| CLI args, env vars | available - `$argc`/`$argv`, `env_*` |
| DateTime | available - the class constructor now runs (#24) |

Still absent: `composite` in Imagick is commented out, and there is no dedicated
byte-buffer type (strings serve, safely).

## Test script status

62 scripts in `test_scripts/`, run with an absolute path, stdin closed, 15s timeout.
**55 pass.** (It was 29/60 before this pass.)

The seven that do not pass cannot pass unattended:

| Script | Needs |
|---|---|
| `readline_test.vs`, `debug_readline.vs`, `simple_readline_test_fixed.vs`, `simple_readline_test.vs`, `direct_test.vs` | interactive stdin; two of them pass when input is piped, the rest use `read_key` and need a real TTY |
| `image_resize.vs`, `image_resize_fixed.vs` | command-line arguments and an image file; they correctly print usage and exit non-zero without them |

Note `include.vs` only works when the interpreter is given an absolute (or at least
directory-qualified) script path - `include` resolves against the parent directory of
the path as given, so a bare filename looks in `/`.

## Syntax reference (verified)

Things that are easy to get wrong, all confirmed by running:

```voidscript
// Every variable takes a $ sigil. Statements end with ;
int $x = 1;
float $f = 2.718;               // numeric literals coerce to the declared type

// Function return type is POSTFIX, after the parameter list
function withValue() int { return 42; }
function noReturn() { return; }
withValue();                    // discarding the value is fine

// No string interpolation, but concat and ternary now work
printnl("x=", $x);              // ok
printnl("x=" + $x);             // ok - string + number concatenates
printnl(format("x={}", $x));    // ok
string $l = $x > 3 ? "big" : "small";   // ternary, only one branch evaluates

// object is the mutable structure; new keys can be added dynamically
object $m = {};
$m->key = "value";

// try/catch/throw, and interpolation in double-quoted strings
try { throw "boom"; } catch (string $e) { printnl("caught $e"); }
printnl('single quotes do not interpolate');

// auto infers, and numeric literals come in every base
auto $anything = 0xFF;          // 0b1010, 0o17, 1e3, 1_000 all lex

// Arrays: read, write, append and member chaining all work
int[] $a = [1, 2, 3];
printnl($a[1]);
$a[0] = 9;
$a[] = 4;
int $sized[3];                  // zero-initialised
printnl($m->items[0]);

// switch needs no trailing ';' and falls through without break
switch ($x) {
    case 1: printnl("one"); break;
    default: printnl("other");
}

// bitwise operators, C precedence
int $flags = 1 | 2 | 4;
if (($flags & 2) == 2) { }
```
