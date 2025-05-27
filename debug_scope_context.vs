// Debug script to check what scope we're in during function execution

function debugScopeFunction(string[] $param) {
    print("=== Function started ===");
    
    // This should work fine
    print("Parameter access works: " + $param[0]);
    
    // Now let's see what scope we're in when operations execute
    print("=== About to execute for loop ===");
    
    // This should reveal the scope issue
    for (string $item : $param) {
        print("In loop: " + $item);
        break;
    }
    
    print("=== Function completed ===");
}

string[] $testParam = ["test"];
debugScopeFunction($testParam);
