// Test to check if variable resolution works correctly within for-loop scope
printnl("Testing C-style for loop variable resolution");

for (int $i = 0; $i < 2; $i++) {
    printnl("Before declaring inner_var, i = ", $i);
    
    // Declare a variable in the loop scope
    int $inner_var = $i + 10;
    
    printnl("After declaring inner_var = ", $inner_var);
    
    // Try to access the loop variable again after declaring inner variable
    printnl("Accessing i again: ", $i);
    
    // Try arithmetic with loop variable
    int $result = $i * 2;
    printnl("Calculation result: ", $result);
}
