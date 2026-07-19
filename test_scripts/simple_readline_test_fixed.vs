# Simple test for individual ReadlineModule functions

printnl("=== Testing readline() ===");
string $input1 = "";
try {
    $input1 = readline("Enter something: ");
} catch {
    printnl("(no input available - skipping interactive test)");
}
printnl("You entered: [" + $input1 + "]");

printnl("\n=== Testing getline() ===");
printnl("Now type something without a prompt:");
string $input2 = "";
try {
    $input2 = getline();
} catch {
    printnl("(no input available - skipping interactive test)");
}
printnl("You entered: [" + $input2 + "]");

printnl("\n=== Testing readchar() ===");
printnl("Press a single key:");
string $char = "";
try {
    $char = readchar();
    printnl("You pressed: [" + $char + "]");
} catch {
    printnl("(no input available - skipping interactive test)");
}

printnl("\n=== Test Complete ===");