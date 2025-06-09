// Test method access control enforcement

class TestMethodAccess {
    private:
    string $private_data = "secret";
    
    function privateMethod() string {
        return "This is private: " + $this->private_data;
    }
    
    public:
    string $public_data = "public";
    
    function publicMethod() string {
        return "This is public: " + $this->public_data;
    }
    
    function callPrivateFromPublic() string {
        // Should work - calling private method from within class
        return "Calling private from public works";
    }
}

TestMethodAccess $obj = new TestMethodAccess();

// Test 1: Public method call - should work
printnl("Test 1 - Public method: ", $obj->publicMethod());

// Test 2: Private method via public method - should work
printnl("Test 2 - Private via public: ", $obj->callPrivateFromPublic());

// Test 3: Direct private method call - should FAIL
printnl("Test 3 - Attempting direct private method call...");
printnl("Result: ", $obj->privateMethod());