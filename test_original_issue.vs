// Test case to reproduce the original scope inheritance issue
function testFunction(int $max) {
    // This should work - function parameter is accessible
    print("Max value is: ", $max);
    
    // This for loop should be able to access $max from the function scope
    for (int $i = 0; $i < $max; $i++) {
        print("Iteration: ", $i);
    }
}

testFunction(3);
