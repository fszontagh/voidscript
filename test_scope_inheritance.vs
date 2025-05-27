// Test script to verify scope inheritance in for loops
// This should test that loop variables can access function parameters

function testForLoopScope (int $param1, int $param2) {
    print("Function parameter param1: ", $param1);
    print("Function parameter param2: ", $param2);
    
    // For loop should be able to access $param1 and $param2
    for (int $i = 0; $i < $param2; $i++) {
        print("Loop iteration ", $i, ", param1 is: ", $param1);
    }
    
    // While loop should also be able to access parameters
    int $j = 0;
    while ($j < $param1) {
        print("While loop iteration ", $j, ", param2 is: ", $param2);
        $j++;
    }
}

// Test the function
print("=== Testing scope inheritance ===");
testForLoopScope(2, 3);
print("=== Test completed ===");
