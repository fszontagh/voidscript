// Test for-in loop variable resolution
printnl("Testing for-in loop variable resolution");

object $items = {
    int $first : 100,
    int $second : 200
};

for (string $key, int $value : $items) {
    printnl("Before declaring local var, key = ", $key, ", value = ", $value);
    
    // Declare a variable in the loop scope
    int $local_var = $value + 5;
    
    printnl("After declaring local_var = ", $local_var);
    
    // Try to access loop variables again
    printnl("Accessing key again: ", $key);
    printnl("Accessing value again: ", $value);
}
