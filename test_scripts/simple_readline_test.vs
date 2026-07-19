# Simple test to check if readline functions exist
print("Testing ReadlineModule functions:");

# Try to call getline to see if it exists
string $input = "";
try {
    $input = getline();
} catch {
    printnl("(no input available - skipping interactive test)");
}
print("Input received: " + $input);