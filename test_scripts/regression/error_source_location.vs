enum C { RED, GREEN = 5 };
switch (C.RED) {
    case "x": printnl("NOT REACHED"); break;
    default: printnl("NOT REACHED");
}
// Regression: bug #15 - runtime errors reported garbage source locations, e.g.
//   [Runtime ERROR] ... at line: 1942890312, column: 14
// with a different value on every run. Interpreter::ExpressionNode declared
//   int line; size_t column;
// with no initialiser, so any node that did not explicitly set them carried stack
// garbage into the error formatter.
//
// This script deliberately fails: it switches on an enum but gives a string case.
// The point is not that it errors, it is WHERE it says the error is - line 3.
// The failing code is kept at the top of the file because the garbage value depends
// on stack layout, and this shape reproduced it on every run.
//
// ctest asserts on the message with PASS_REGULAR_EXPRESSION, which makes the non-zero
// exit code irrelevant.
