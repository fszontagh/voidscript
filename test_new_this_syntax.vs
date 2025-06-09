// Test script for new $this syntax requirements
// This script should test both valid $this usage and error cases with bare this

class TestClass {
    public:
        string name;
        int value;
        
    function construct(string $n, int $v) {
        // Valid: Use $this for class member access
        $this->name = $n;
        $this->value = $v;
    }
    
    function getName() {
        // Valid: Return using $this
        return $this->name;
    }
    
    function setValue(int $v) {
        // Valid: Assignment using $this
        $this->value = $v;
    }
    
    function invalidThisUsage() {
        // This should cause a parse error - bare 'this' is not allowed
        // Uncomment the next line to test error handling:
        // this->name = "invalid";
        
        // This should also cause an error:
        // return this->value;
    }
}

// Test usage
TestClass $obj = new TestClass("test", 42);
string $result = $obj->getName();
print($result);

// Test method calls
$obj->setValue(100);
print("setValue completed");