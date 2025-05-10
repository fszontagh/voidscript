// Main test file for include functionality
print("Starting include test\n");

// Include a file that adds new variables
print("Before include statement\n");
include "include_test_1.vs";
print("After include statement\n");

// Try to use the included variables and function
print("Trying to access included_var...\n");
print("Included variable: $included_var = ", $included_var, "\n");
print("Trying to access j...\n");
print("Included variable: $j = ", $j, "\n");
print("Trying to call add function...\n");
print("Using included function: add(5, 3) = ", add(5, 3), "\n");

print("Include test completed successfully!\n"); 