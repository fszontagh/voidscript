class TestClass {
    private:
    string $privateProperty = "Private Value";
    
    public:
    string $publicProperty = "Public Value";
    
    function getPrivateProperty() string {
        return this->$privateProperty;
    }
}

TestClass $obj = new TestClass();

// Test 1: Public property access (should work)
printnl("Test 1 - Public property: ", $obj->$publicProperty);

// Test 2: Private property access via method (should work)
printnl("Test 2 - Private via method: ", $obj->getPrivateProperty());

printnl("=== Valid access tests passed ===");

// Test 3: Direct private property access (should fail)
printnl("Test 3 - Attempting direct private property access...");
string $private = $obj->$privateProperty;
printnl("ERROR: This line should never be reached!");