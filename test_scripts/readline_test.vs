# Test script for ReadlineModule functionality

printnl("=== VoidScript ReadlineModule Test ===\n");

# Test 1: Basic readline with prompt
printnl("Test 1: readline() with prompt");
printnl("Please enter your name: ");
string $name = readline("Name: ");
printnl("Hello, " + $name + "!\n");

# Test 2: getline() without prompt
printnl("Test 2: getline() without prompt");
printnl("Enter something (no prompt will be shown): ");
string $input = getline();
printnl("You entered: '" + $input + "'\n");

# Test 3: readchar() for single character input
printnl("Test 3: readchar() for single character");
printnl("Press any key (no Enter needed): ");
string $char = readchar();
int $number = string_to_number($char);
printnl("You pressed char: ", $char, " converted to number: ", $number);

# Test 4: Interactive menu example
printnl("=== Interactive Menu Example ===");
printnl("1. Option 1");
printnl("2. Option 2");
printnl("3. Exit");
string $choice = readline("Select option (1-3): ");

if ($choice == "1") {
    printnl("You selected Option 1");
} else if ($choice == "2") {
    printnl("You selected Option 2");
} else if ($choice == "3") {
    printnl("Goodbye!");
} else {
    printnl("Invalid choice: " + $choice);
}

printnl("\n=== ReadlineModule Test Complete ===");