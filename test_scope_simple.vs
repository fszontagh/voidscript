// Simple test for scope issue

function testScope(string $param) {
    print("Function param: " + $param);
    
    // Test for-in loop access to function parameter
    string[] $items = ["a", "b"];
    for (string $item : $items) {
        print("In loop, param is: " + $param);
        break;
    }
}

testScope("test");
