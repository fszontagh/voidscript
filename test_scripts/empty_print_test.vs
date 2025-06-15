// Test file to reproduce the print("") and printnl("") issue
// This test helps identify if quotation marks are being output instead of expected behavior

// Expected behavior documentation:
// - print("") should output nothing (no characters, no quotes)
// - printnl("") should output just a newline character  
// - print("a") + print("") + print("b") should output "ab" on same line
// - printnl("test") + printnl("") + printnl("test2") should have empty line between test and test2

printnl("=== EMPTY PRINT TEST CASES ===");
printnl("");

// Test Case 1: print("") - should output nothing
printnl("Test 1: print(\"\") - should output nothing:");
printnl("Before print(\"\")");
print("");
printnl("After print(\"\")");
printnl("");

// Test Case 2: printnl("") - should output just a newline
printnl("Test 2: printnl(\"\") - should output just a newline:");
printnl("Before printnl(\"\")");
printnl("");
printnl("After printnl(\"\")");
printnl("");

// Test Case 3: print("a") + print("") + print("b") - should output "ab" on same line
printnl("Test 3: print(\"a\") + print(\"\") + print(\"b\") - should output \"ab\":");
print("a");
print("");
print("b");
printnl("");
printnl("");

// Test Case 4: printnl with empty line between
printnl("Test 4: printnl(\"test\") + printnl(\"\") + printnl(\"test2\") - empty line between:");
printnl("test");
printnl("");
printnl("test2");
printnl("");

// Test Case 5: Multiple empty prints
printnl("Test 5: Multiple empty prints:");
print("start");
print("");
print("");
print("");
print("end");
printnl("");
printnl("");

// Test Case 6: Mixed empty and non-empty
printnl("Test 6: Mixed empty and non-empty:");
print("A");
print("");
print("B");
print("");
print("C");
printnl("");
printnl("");

// Test Case 7: Check variable assignment with empty string
printnl("Test 7: Variable assignment with empty string:");
string $empty = "";
printnl("Variable content between brackets: [" + $empty + "]");
print("Direct print of variable: ");
print($empty);
printnl(" <-- should be nothing between colon and arrow");
printnl("");

// Test Case 8: String concatenation with empty
printnl("Test 8: String concatenation with empty:");
string $test = "Hello" + "" + "World";
printnl("Concatenated result: " + $test);
printnl("");

printnl("=== END OF TESTS ===");