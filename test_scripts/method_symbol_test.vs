// Method symbol test script
// This tests the functionality of CLASS and METHOD symbol kinds

// Define a simple class with methods
class MethodTest {
    public: string $name;
    public: int $value;
    
    // Constructor
    function construct(string $name_arg, int $value_arg) {
        $this->name = $name_arg;
        $this->value = $value_arg;
        printnl("Constructor called with name: ", $name_arg, " and value: ", $value_arg);
    }
    
    // Regular method
    function display() {
        printnl("MethodTest object - Name: ", $this->name, ", Value: ", $this->value);
    }
    
    // Method with return type
    function getName() string {
        printnl("getName method called");
        return $this->name;
    }
    
    // Method with parameters
    function setValue(int $new_value) {
        $this->value = $new_value;
        printnl("Value updated to: ", $new_value);
    }
}

// Create an instance of the class
printnl("Creating instance of MethodTest...");
MethodTest $test = new MethodTest("Test Object", 42);

// Debug: Check the class metadata in the object
printnl("Debug - $class_name property: ", $test["$class_name"]);

// Call methods on the instance
printnl("\nTesting method calls:");
$test->display();

// Test method with return value
string $name = $test->getName();
printnl("Retrieved name: ", $name);

// Test method with parameter
printnl("\nUpdating value...");
$test->setValue(100);
$test->display();
