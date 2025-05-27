// Test to check if for-loop variables are accessible within the loop
int $counter = 0;

for (int $i = 0; $i < 3; $i++) {
    // This should work - accessing the for-loop variable within the loop
    printnl("Loop iteration: ", $i);
    
    // Create a variable inside the loop
    int $inner_var = $i * 2;
    
    // This should work - accessing a variable declared in the loop scope
    printnl("Inner variable: ", $inner_var);
    
    $counter++;
}

printnl("Counter after loop: ", $counter);

// This SHOULD fail - trying to access loop variable outside loop
// printnl("i outside loop: ", $i);

// This SHOULD fail - trying to access inner variable outside loop  
// printnl("inner_var outside loop: ", $inner_var);
