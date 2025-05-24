// Debug test script for method symbols

// Define a simple test class
class DebugMethodTest {
    public: string $name;
    public: int $value;
    
    // Constructor
    function construct(string $name_arg, int $value_arg) {
        this->$name = $name_arg;
        this->$value = $value_arg;
        printnl("Constructor called with name: ", $name_arg, " and value: ", $value_arg);
    }
    
    // Regular method
    function display() {
        printnl("DebugMethodTest object - Name: ", this->$name, ", Value: ", this->$value);
    }
    
    // Method with return type
    function getName() string {
        printnl("getName method called");
        return this->$name;
    }
    
    // Method with parameters
    function setValue(int $new_value) {
        this->$value = $new_value;
        printnl("Value updated to: ", $new_value);
    }
    
    // Method with boolean return type
    function isValueGreaterThan(int $compare_value) bool {
        printnl("Comparing ", this->$value, " > ", $compare_value);
        return this->$value > $compare_value;
    }
}

// Create an instance of the class
printnl("\n=== Creating instance of DebugMethodTest ===");
DebugMethodTest $test = new DebugMethodTest("Debug Object", 42);

// Show the object's class properties
printnl("\n=== Object properties ===");
// Note: Class internal properties like $class_name are not accessible from user scripts

// Call methods on the instance
printnl("\n=== Testing method calls ===");
$test->display();

// Test method with return value
printnl("\n=== Testing method with return value ===");
string $name = $test->getName();
printnl("Retrieved name: ", $name);

// Test method with parameter
printnl("\n=== Testing method with parameter ===");
$test->setValue(100);
$test->display();

// Test boolean method
printnl("\n=== Testing method with boolean return value ===");
bool $isGreater = $test->isValueGreaterThan(50);
printnl("Is value greater than 50? ", $isGreater);

$isGreater = $test->isValueGreaterThan(150);
printnl("Is value greater than 150? ", $isGreater);

printnl("\n=== Test completed successfully ===");
