int $counter = 0;

printnl("Starting while loop test...");

while ($counter < 5) {
    printnl("While loop counter: ", $counter);
    $counter++;
}

printnl("While loop finished. Final counter: ", $counter); // Expected: 5

// Test with condition initially false
int $false_counter = 10;
while ($false_counter < 5) {
    printnl("This should not print.");
    $false_counter++;
}
printnl("False condition loop finished. Counter: ", $false_counter); // Expected: 10 