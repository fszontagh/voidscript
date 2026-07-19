// Regression: bug #23 - `auto` was only legal as a for-each value type. Using it as a
// general declaration failed with "Type mismatch for variable 'x': expected 'auto' but
// got 'object'", which is what blocked module_exists_comprehensive_test.vs
// (`auto $modules = list_modules();`).
//
// `auto` now takes the type of its initialiser, as in C++.
// Expected: clean exit 0.

auto $i = 42;
auto $s = "text";
auto $d = 3.5;
auto $b = true;

printnl($i);                    // 42
printnl($s);                    // text
printnl($d);                    // 3.500000
printnl($b);                    // true

// the inferred type is the real one, so it participates normally
printnl($i + 1);                // 43
printnl($s + "!");              // text!

// objects and arrays
auto $o = { string k: "v" };
printnl($o->k);                 // v
auto $arr = [1, 2, 3];
printnl($arr[1]);               // 2

// from a function call - the case module_exists_comprehensive_test.vs needed
auto $mods = list_modules();
printnl(sizeof($mods) > 0);     // true

// inside a function
function makeIt() int { return 7; }
auto $called = makeIt();
printnl($called);               // 7

// still legal in for-each, its original home
for (string $k, auto $v : $o) { printnl($k); }   // k

printnl("done");
