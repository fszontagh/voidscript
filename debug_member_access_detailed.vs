// Simple test to debug private property access
class TestClass {
    private:
    int $age = 25;
    
    public:
    function construct() {
        // Empty constructor
    }
}

TestClass $obj = new TestClass();
printnl("Trying to access private property...");
printnl("Value: ", $obj->age);