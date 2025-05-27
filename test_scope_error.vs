// Test to check scoping behavior - this should show the actual error
int $counter = 0;

for (int $i = 0; $i < 3; $i++) {
    printnl("Loop iteration: ", $i);
    int $inner_var = $i * 2;
    $counter++;
}

// This SHOULD fail - trying to access loop variable outside loop
printnl("i outside loop: ", $i);
