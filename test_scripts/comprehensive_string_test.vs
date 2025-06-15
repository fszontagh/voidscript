// Comprehensive test for string literal edge cases
print("Test 1: Normal string");
printnl("");
print("Test 2: Empty string between: ");
print("");
printnl(" <- should be nothing");

// Test with variable assignment
string $empty = "";
string $normal = "hello";
print("Test 3: Variable empty: [");
print($empty);
print("] Variable normal: [");
print($normal);
printnl("]");

// Test with escaped characters in empty string (shouldn't happen but let's be sure)
printnl("Test 4: Should be just a newline below:");
printnl("");
print("Test 5: End");