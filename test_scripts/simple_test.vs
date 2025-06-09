// Simplified test for $this->property access
class TestClass {
    private: int $x = 10;
    
    function getX() int {
        return $this->x;
    }
}

TestClass $test = new TestClass();
printnl("Value of x: ", $test->getX());
