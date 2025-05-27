// Simple test for scope inheritance
function testScope(int $param) {
    print("Parameter value: ", $param);
    for (int $i = 0; $i < $param; $i++) {
        print("Loop iteration: ", $i);
    }
}

testScope(2);
