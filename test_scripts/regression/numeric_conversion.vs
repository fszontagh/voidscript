// Regression: bug #10 - declaring a variable with a numeric type failed whenever the
// initialiser had a different numeric type. All float literals lex as double, so even
// `float $f = 2.718;` was rejected with
//   Type mismatch for variable 'f': expected 'float' but got 'double'
//
// Conversions between numeric types are now allowed, EXCEPT float/double -> int, which
// would silently drop the fractional part.
// Expected: clean exit 0.

// double literal into a float
float $f = 2.718;
printnl($f);

// int literal into the wider types
double $d = 3;
printnl($d);
float $f2 = 5;
printnl($f2);

// float into double
float $src = 1.5;
double $widened = $src;
printnl($widened);

// same-type declarations must keep working
int $i = 42;
printnl($i);
double $d2 = 3.14;
printnl($d2);

printnl("done");
