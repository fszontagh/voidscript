// Test script to verify method call fix works for multiple scenarios

class Calculator {
    int $value;
    string $name;
    
    // Constructor to initialize values properly
    function construct() {
        $this->value = 42;
        $this->name = "Default Calculator";
        printnl("Constructor: Setting value to 42 and name to Default Calculator");
    }
    
    // Getter method
    function getValue() int {
        return $this->value;
    }
    
    // Setter method  
    function setValue(int $newValue) null {
        $this->value = $newValue;
    }
    
    // Method with string return
    function getName() string {
        return $this->name;
    }
    
    // Method with calculations
    function add(int $num) int {
        $this->value = $this->value + $num;
        return $this->value;
    }
}

// Test the fix
printnl("=== Testing Method Fix ===");

Calculator $calc = new Calculator();

printnl("Name: ", $calc->getName());
printnl("Initial value: ", $calc->getValue());

$calc->add(5);
printnl("After adding 5: ", $calc->getValue());

$calc->setValue(20);
printnl("After setting to 20: ", $calc->getValue());
