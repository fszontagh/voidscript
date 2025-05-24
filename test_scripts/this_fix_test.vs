// Test script for fixing 'this' keyword access in methods

class TestThisAccess {
    // Define some properties
    public: string $name = "Default Name";
    public: int $value = 42;
    
    // Constructor - explicitly sets properties
    function construct(string $name_arg, int $value_arg) {
        this->$name = $name_arg;
        this->$value = $value_arg;
        printnl("Constructor called - Name set to: ", this->$name, ", Value set to: ", this->$value);
    }
    
    // Simple method that uses this
    function display() {
        printnl("TestThisAccess object:");
        printnl("  - Name: '", this->$name, "'");
        printnl("  - Value: ", this->$value);
    }
    
    // Method that returns a property
    function getName() string {
        return this->$name;
    }
    
    // Method that modifies a property
    function setValue(int $new_value) {
        this->$value = $new_value;
    }
}

// Create an instance with constructor
printnl("\n=== Creating object with constructor ===");
TestThisAccess $test = new TestThisAccess("Test Object", 100);

// Display the object properties
printnl("\n=== Calling display() method ===");
$test->display();

// Test the getter method
printnl("\n=== Testing getName() method ===");
string $name = $test->getName();
printnl("Retrieved name: '", $name, "'");

// Test property modification
printnl("\n=== Testing setValue() method ===");
$test->setValue(200);
printnl("After setValue(200), calling display():");
$test->display();

// Create another instance (tests defaults)
printnl("\n=== Creating object with default values ===");
TestThisAccess $test2 = new TestThisAccess("Second Object", 500); 
$test2->display();

printnl("\n=== Test completed successfully ===");
