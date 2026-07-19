// Regression: bug #9 - `"x=" + $x` threw
//   Unsupported types in binary expression: string and int
// which made the single most natural way to build a message an error, and forced
// every call site to reach for format() or number_to_string().
//
// `+` already means concatenation when both sides are strings, so extending it to
// "string on one side" is consistent. It applies to `+` only: comparing a string
// with a number is still a type error rather than a silent stringify.
// Expected: clean exit 0.

int $i = 42;
double $d = 3.5;
boolean $b = true;

printnl("i=" + $i);             // i=42
printnl($i + "=i");             // 42=i
printnl("d=" + $d);             // d=3.500000
printnl("b=" + $b);             // b=true

// chained
printnl("a" + $i + "b");        // a42b

// string + string still works
printnl("x" + "y");             // xy

// numeric + numeric must stay arithmetic, NOT concatenation
printnl(2 + 3);                 // 5
printnl(2.5 + 1);               // 3.500000

// comparisons across types remain an error, so they are not exercised here.

printnl("done");
