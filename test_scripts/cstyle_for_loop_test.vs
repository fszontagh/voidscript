printnl("Starting C-style for loop test...");

for (int $i = 0; $i < 3; $i++) {
    printnl("C-style loop counter: ", $i);
}

printnl("C-style loop finished.");

const string $const_string = "Hello, World!";
printnl("Constant string: ", $const_string);
// Test with empty increment
printnl("Testing C-style loop with empty increment...");
for (int $j = 5; $j > 2; ) {
    printnl("C-style (no incr) counter: ", $j);
    $j--; // Manual decrement inside body
}
printnl("C-style loop (no incr) finished.");
printnl("Constant string: ", $const_string);

// Test access to outer scope variable
int $outer_var = 100;
for (int $k = 0; $k < 2; $k++) {
    printnl("Outer var inside loop: ", $outer_var);
    $outer_var++;
}
printnl("Outer var after loop: ", $outer_var); // Expected: 102 