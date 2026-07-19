// Regression: bug #22 - a C-style for loop declared its induction variable in the
// ENCLOSING scope, so a second loop reusing the name failed with
//   Variable 'i' already declared in scope '...'
// Using `$i` twice in one file is completely ordinary, and this is what
// enum_comprehensive.vs tripped over.
// Expected: clean exit 0.

for (int $i = 0; $i < 2; $i++) {
    printnl("a$i");                 // a0 a1
}

// the same name again, at the same level
for (int $i = 0; $i < 2; $i++) {
    printnl("b$i");                 // b0 b1
}

// and a third time, nested inside another loop
for (int $j = 0; $j < 2; $j++) {
    for (int $i = 0; $i < 2; $i++) {
        printnl("c$j$i");           // c00 c01 c10 c11
    }
}

// a for-each reusing a name too
string[] $words = ["x", "y"];
for (string $w : $words) { printnl("d$w"); }
for (string $w : $words) { printnl("e$w"); }

// while loops with a body-local declaration, run twice
int $n = 0;
while ($n < 2) {
    int $tmp = $n;
    printnl("f$tmp");               // f0 f1
    $n = $n + 1;
}

printnl("done");

// A bare (undeclared) induction variable assigns to the enclosing one, so it is
// still readable after the loop. This is the only way to observe the counter now
// that a DECLARED induction variable is scoped to the loop.
int $k = 0;
for ($k = 0; $k < 3; $k++) { }
printnl("k=$k");                    // k=3
printnl("scope-done");
