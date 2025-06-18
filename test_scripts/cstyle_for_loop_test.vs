printnl("Testing C-style for loop");

for (int $i = 0; $i < 3; $i++) {
    printnl("Loop iteration: ", $i);
}

printnl("C-style for loop completed");

// Nested C-style for loop test
for (int $outer = 0; $outer < 2; $outer++) {
    for (int $inner = 0; $inner < 2; $inner++) {
        printnl("Outer: ", $outer, " Inner: ", $inner);
    }
}

printnl("Nested C-style for loops completed");