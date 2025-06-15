// Quick test to reproduce the empty print issue
// This minimal test focuses on the core problem

// Test 1: print("") should output nothing
print("START");
print("");
printnl("END");

// Test 2: printnl("") should output just newline
printnl("Line1");
printnl("");
printnl("Line3");

// Test 3: Variable test
string $empty = "";
print("Variable test: [");
print($empty);
printnl("]");