# Simple test for individual ReadlineModule functions

printnl("=== Testing readline() ===");
string $input1 = readline("Enter something: ");
printnl("You entered: [" + $input1 + "]");

printnl("\n=== Testing getline() ===");
printnl("Now type something without a prompt:");
string $input2 = getline();
printnl("You entered: [" + $input2 + "]");

printnl("\n=== Testing readchar() ===");
printnl("Press a single key:");
string $char = readchar();
printnl("You pressed: [" + $char + "]");

printnl("\n=== Test Complete ===");