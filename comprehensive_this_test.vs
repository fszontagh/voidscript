// Comprehensive test for $this-> syntax implementation
class ComprehensiveTest {
    // Test different property types
    private: string $name = "Default";
    private: int $value = 42;
    private: bool $active = true;
    
    // Test property access
    function getName() string {
        return $this->name;
    }
    
    function getValue() int {
        return $this->value;
    }
    
    function isActive() bool {
        return $this->active;
    }
    
    // Test property assignment
    function setName(string $newName) {
        $this->name = $newName;
    }
    
    function setValue(int $newValue) {
        $this->value = $newValue;
    }
    
    function setActive(bool $status) {
        $this->active = $status;
    }
    
    // Test arithmetic operations with $this
    function incrementValue() int {
        $this->value = $this->value + 1;
        return $this->value;
    }
    
    function multiplyValue(int $multiplier) int {
        $this->value = $this->value * $multiplier;
        return $this->value;
    }
    
    // Test method chaining
    function chainTest() string {
        $this->name = "Chained";
        $this->value = 100;
        return $this->name;
    }
    
    // Test complex expressions with $this
    function complexExpression() int {
        return ($this->value * 2) + 10;
    }
}

printnl("=== COMPREHENSIVE $this-> SYNTAX TEST ===");

// Create instance
ComprehensiveTest $test = new ComprehensiveTest();

// Test basic property access
printnl("1. Basic property access:");
printnl("   Name: ", $test->getName());
printnl("   Value: ", $test->getValue());
printnl("   Active: ", $test->isActive());

// Test property modification
printnl("2. Property modification:");
$test->setName("Modified");
$test->setValue(99);
$test->setActive(false);
printnl("   New name: ", $test->getName());
printnl("   New value: ", $test->getValue());
printnl("   New active: ", $test->isActive());

// Test arithmetic operations
printnl("3. Arithmetic operations:");
printnl("   Incremented value: ", $test->incrementValue());
printnl("   Multiplied by 3: ", $test->multiplyValue(3));

// Test method chaining
printnl("4. Method chaining:");
printnl("   Chain result: ", $test->chainTest());
printnl("   Final value: ", $test->getValue());

// Test complex expressions
printnl("5. Complex expressions:");
printnl("   Complex result: ", $test->complexExpression());

printnl("=== TEST COMPLETED SUCCESSFULLY ===");