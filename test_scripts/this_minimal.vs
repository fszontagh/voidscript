// Minimal test for $this->property comparison
class MinTest {
    private: int $value = 20;
    
    function isAdult() bool {
        // First print the value 
        printnl("DEBUG: Attempting to access property value");
        printnl("DEBUG: Property value is: ", $this->value);
        
        // Now do the comparison with the property value
        bool $result = $this->value >= 18;
        printnl("DEBUG: Comparison result: ", $result);
        return $result;
    }
}

// Create instance
MinTest $t = new MinTest();

// Test comparison
printnl("Is adult: ", $t->isAdult()); 