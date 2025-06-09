// Simple test to understand property naming
class SimpleTest {
    int $testInt = 42;
    
    function construct() {
        printnl("Constructor called");
    }
    
    function getTestInt() int {
        return $this->testInt;
    }
}

SimpleTest $obj = new SimpleTest();
printnl("Result: ", $obj->getTestInt());
