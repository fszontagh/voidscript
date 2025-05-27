// Debug script to investigate function parameter scope accessibility
// This will help us understand the scope hierarchy during function execution

function debugScopes(array $texts) {
    print("=== Inside debugScopes function ===");
    print("Function parameter 'texts' type: " + typeof($texts));
    print("Function parameter 'texts' size: " + size($texts));
    
    // Try to access the parameter directly
    print("Direct access to texts parameter works: " + $texts[0]);
    
    // Now let's see what happens in a for loop
    print("\n=== Starting for loop ===");
    for (string $item : $texts) {
        print("Loop item: " + $item);
        break; // Exit after first iteration to avoid long output
    }
    print("=== For loop completed ===");
}

// Test the function
string[] $testArray = ["item1", "item2", "item3"];
print("=== Calling debugScopes function ===");
debugScopes($testArray);
