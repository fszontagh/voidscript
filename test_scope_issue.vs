// Test for scope issue with function parameters in for loops

function testForInScope(string[] $param) {
    printnl("Function started with param length: ", array_length($param));
    
    // Test 1: For-in loop accessing function parameter
    for (string $item : $param) {
        printnl("In for-in loop, item: ", $item);
        printnl("In for-in loop, param length: ", array_length($param));
        break;
    }
}

function testCStyleForScope(int $count) {
    printnl("C-style for function started with count: ", $count);
    
    // Test 2: C-style for loop accessing function parameter  
    for (int $i = 0; $i < $count; $i++) {
        printnl("In C-style for loop, i: ", $i);
        printnl("In C-style for loop, count: ", $count);
        break;
    }
}

string[] $testArray = ["item1", "item2"];
testForInScope($testArray);
testCStyleForScope(3);
