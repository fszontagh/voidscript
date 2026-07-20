// Regression: task #5 - `continue` did not exist at all (not a keyword, not parsed:
// "Identifier 'continue' not found"), and a `break` outside any loop escaped to the
// catch-all backstop as "Internal error: unhandled exception" instead of naming the
// problem. Both are now proper language features with a proper diagnostic.
// Expected: clean exit 0.

// --- continue in a C-style for ---
for (int $i = 0; $i < 5; $i++) {
    if ($i == 2) { continue; }
    printnl("a$i");                 // a0 a1 a3 a4
}

// --- continue in a while: the increment must still run, or this hangs ---
int $n = 0;
while ($n < 5) {
    $n = $n + 1;
    if ($n == 3) { continue; }
    printnl("b$n");                 // b1 b2 b4 b5
}

// --- continue in a for-each ---
string[] $items = ["x", "skip", "y"];
for (string $it : $items) {
    if ($it == "skip") { continue; }
    printnl("c$it");                // cx cy
}

// --- break still works, and beats continue in the same loop ---
for (int $j = 0; $j < 10; $j++) {
    if ($j == 1) { continue; }
    if ($j == 3) { break; }
    printnl("d$j");                 // d0 d2
}

// --- nested: continue applies to the innermost loop only ---
for (int $o = 0; $o < 2; $o++) {
    for (int $k = 0; $k < 3; $k++) {
        if ($k == 1) { continue; }
        printnl("e$o$k");           // e00 e02 e10 e12
    }
}

// --- continue inside a try still reaches the loop ---
for (int $t = 0; $t < 3; $t++) {
    try {
        if ($t == 1) { continue; }
        printnl("f$t");             // f0 f2
    } catch (string $e) {
        printnl("NOT REACHED");
    }
}

printnl("done");
