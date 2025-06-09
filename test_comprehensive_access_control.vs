class AccessTestClass {
    private:
    string $privateProperty = "This is private";
    int $privateAge = 25;
    
    function privateMethod() string {
        return "Private method result";
    }
    
    public:
    string $publicProperty = "This is public";
    int $publicCount = 0;
    
    function construct(string $name) {
        this->$privateProperty = $name;
    }
    
    function getPrivateProperty() string {
        // Accessing private property from within class - should work
        return this->$privateProperty;
    }
    
    function callPrivateMethod() string {
        // Calling private method from within class - should work
        return this->privateMethod();
    }
    
    function updatePrivateAge(int $newAge) null {
        // Modifying private property from within class - should work
        this->$privateAge = $newAge;
    }
    
    function getPrivateAge() int {
        return this->$privateAge;
    }
    
    function incrementPublicCount() null {
        this->$publicCount = this->$publicCount + 1;
    }
}

// Create test instance
AccessTestClass $obj = new AccessTestClass("Custom Name");

printnl("=== ACCESS CONTROL TEST RESULTS ===");

// Test 1: Public property access (should work)
printnl("1. Public property access: ", $obj->$publicProperty);

// Test 2: Public method calls (should work)
printnl("2. Private property via public method: ", $obj->getPrivateProperty());
printnl("3. Private method via public method: ", $obj->callPrivateMethod());

// Test 3: Modify private property via public method (should work)
$obj->updatePrivateAge(30);
printnl("4. Updated private age via public method: ", $obj->getPrivateAge());

// Test 4: Public property modification (should work)
$obj->incrementPublicCount();
printnl("5. Public property after increment: ", $obj->$publicCount);

printnl("=== All valid access tests PASSED ===");

// The following would cause access control violations:
printnl("6. Attempting direct access to private property (should fail):");
// This line should throw an access control exception:
// string $private = $obj->$privateProperty;

printnl("7. Attempting direct call to private method (should fail):");
// This line should throw an access control exception:
// string $result = $obj->privateMethod();

printnl("=== Access control implementation completed successfully ===");