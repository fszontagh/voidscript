// Test to isolate the scope issue - comparing simple access vs for loop

function testSimpleAccess(string[] $param) {
    print("=== Function started ===");
    
    // Test 1: Direct variable access
    print("Direct access test:");
    print("Parameter exists: " + typeof($param));
    
    // Test 2: Using in expression
    string $temp = "Prefix: " + typeof($param);
    print("Expression test: " + $temp);
    
    // Test 3: Simple array access
    print("Array access test: " + $param[0]);
    
    print("=== All simple tests passed ===");
    
    // Test 4: This should fail according to our hypothesis
    print("=== About to test for loop (this should fail) ===");
    for (string $item : $param) {
        print("Should not reach here");
        break;
    }
}

// Run the test
string[] $data = ["item"];
testSimpleAccess($data);
