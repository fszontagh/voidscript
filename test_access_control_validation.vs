// Test access control enforcement - this should demonstrate both successful access and failures

class TestAccessControl {
    private:
    string $private_name = "Secret";
    int $private_age = 25;
    
    function getPrivateData() string {
        // This should work - accessing private from within class
        return "Name: " + $this->private_name + ", Age: " + $this->private_age;
    }
    
    public:
    string $public_info = "Public Information";
    
    function getPublicInfo() string {
        return $this->public_info;
    }
    
    function getPrivateViaPublic() string {
        // Public method accessing private - should work
        return $this->private_name;
    }
}

TestAccessControl $obj = new TestAccessControl();

// Test 1: Public property access - should work
printnl("Test 1 - Public property access: ", $obj->public_info);

// Test 2: Public method call - should work  
printnl("Test 2 - Public method call: ", $obj->getPublicInfo());

// Test 3: Private access via public method - should work
printnl("Test 3 - Private via public method: ", $obj->getPrivateViaPublic());

// Test 4: Direct private property access - should FAIL
printnl("Test 4 - Attempting direct private access...");
printnl("Private name: ", $obj->private_name);