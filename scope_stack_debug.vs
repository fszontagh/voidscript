// Debug script to check scope stack during function execution
// This will help us see exactly what scopes are active

function scopeDebugFunction(string[] $param) {
    print("=== Function start - parameter access test ===");
    
    // Test 1: Can we access the parameter directly?
    print("Direct parameter access test:");
    print("Parameter type: " + typeof($param));
    print("Parameter size: " + size($param));
    print("First item: " + $param[0]);
    
    print("\n=== About to start for loop ===");
    print("If this works but the for loop fails, we know the issue is in the for loop scope handling");
    
    // Test 2: This should fail if there's a scope issue
    for (string $item : $param) {
        print("Loop iteration with item: " + $item);
        break; // Exit immediately to avoid long output
    }
    
    print("=== For loop completed successfully ===");
}

// Call the function
string[] $testData = ["test1", "test2", "test3"];
print("=== Starting scope debug test ===");
scopeDebugFunction($testData);
print("=== Test completed ===");
