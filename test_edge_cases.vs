// Edge case testing for access control and syntax compatibility

class EdgeCaseTest {
    private:
    string $private_prop = "private_value";
    bool $private_bool = true;
    
    public:
    string $public_prop = "public_value";
    bool $public_bool = false;
    
    // Test this-> syntax variations
    function testThisAccess() {
        printnl("Testing this-> access within class:");
        printnl("  Private prop: ", this->private_prop);
        printnl("  Private bool: ", this->private_bool);
        printnl("  Public prop: ", this->public_prop);
        printnl("  Public bool: ", this->public_bool);
    }
    
    function getPrivateProp() string {
        return this->private_prop;
    }
    
    function getPrivateBool() bool {
        return this->private_bool;
    }
}

EdgeCaseTest $obj = new EdgeCaseTest();

printnl("=== EDGE CASE TESTING ===");

// Test 1: this-> access within methods (should work)
printnl("\nTest 1 - Internal this-> access:");
$obj->testThisAccess();

// Test 2: Public property access (should work)
printnl("\nTest 2 - Public property access:");
printnl("Public prop: ", $obj->public_prop);
printnl("Public bool: ", $obj->public_bool);

// Test 3: Private access via public methods (should work)
printnl("\nTest 3 - Private access via public methods:");
printnl("Private via method: ", $obj->getPrivateProp());
printnl("Private bool via method: ", $obj->getPrivateBool());

printnl("\n=== ACCESS CONTROL TESTS ===");

// Test 4: Try accessing private properties directly (should fail or return null)
printnl("\nTest 4 - Direct private property access attempts:");
printnl("Attempting to access private_prop...");
// This should demonstrate the access control behavior