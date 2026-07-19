// Regression: bug #6 - a sized array declaration was a syntax error:
//   Expected token PUNCTUATION with value ';': Token { Type: PUNCTUATION, Value: "[" }
// This is what broke enum_advanced.vs, enum_comprehensive.vs and enum_switch.vs,
// all of which declare `int $x[N];`.
//
// A sized declaration now creates an array of N default-initialised elements.
// Expected: clean exit 0.

int $codes[3];
printnl($codes[0]);             // 0 - default initialised
$codes[0] = 200;
$codes[2] = 404;
printnl($codes[0]);             // 200
printnl($codes[2]);             // 404

string $names[2];
$names[1] = "second";
printnl($names[1]);             // second

double $ratios[2];
printnl($ratios[1]);            // 0.000000

printnl("done");
