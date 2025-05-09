// Main test file for include functionality
// This file will include other files and test various scenarios

// Test 1: Basic variable inclusion
int $i = 42;
string $var = "main file variable";

print("Main file variables:\n");
print("$i = ", $i, "\n");
print("$var = ", $var, "\n");
print("\n");

// Include a file that adds new variables
include "test_scripts/include_test_1.vs";

print("After including test_scripts/include_test_1.vs:\n");
print("$included_var = ", $included_var, "\n");
print("$j = ", $j, "\n");
print("add(5, 3) = ", add(5, 3), "\n");
print("\n");

// Test 3: Error case - trying to redefine a variable
include "test_scripts/include_test_2.vs";  // This should fail with a symbol conflict error 

// This should not be reached due to the error above
print("This should not be printed\n");
print($var); 