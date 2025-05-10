// Main test file that includes another file
include "include_test_1.vs";

// Use variables from the included file
string test_var = "Main file variable";
int i = 42;

// Print variables from both files
print("From main file: " + test_var);
print("From included file: " + included_var);
print("i = " + i);
print("j = " + j);

// Call function from included file
int result = add(i, j);
print("add(" + i + ", " + j + ") = " + result); 