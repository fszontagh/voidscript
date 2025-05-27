function loopOverArrayDebug(string[] $texts) {
    print("=== DEBUG: Starting loopOverArray function ===");
    print("Current scope: " + getCurrentScope());
    print("Parameter $texts received: " + $texts.toString());
    
    // Try to access the parameter directly first
    print("Direct access to $texts works");
    
    print("=== DEBUG: About to start for loop ===");
    for (string $r : $texts) {
        print("Loop iteration: " + $r);
    }
    print("=== DEBUG: For loop completed ===");
}

function getCurrentScope() {
    // This won't work, but let's try a simple approach first
    return "scope_info_placeholder";
}

print("=== Starting debug test ===");
string[] $test_texts = ["item1", "item2", "item3"];
loopOverArrayDebug($test_texts);
print("=== Debug test completed ===");
