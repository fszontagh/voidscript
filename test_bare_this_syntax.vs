// Test file to verify bare 'this' usage is rejected with helpful error messages
class TestClass {
    private: string $name = "Test";
    
    function badMethod1() string {
        // This should fail - using bare 'this' instead of '$this'
        return this->name;
    }
    
    function badMethod2() {
        // This should also fail - using bare 'this' instead of '$this'
        this->name = "New Value";
    }
    
    function goodMethod() string {
        // This should work - using correct '$this' syntax
        return $this->name;
    }
}

// Create instance
TestClass $obj = new TestClass();

// Try to call the bad methods
printnl("Testing bare this usage...");
$obj->badMethod1();