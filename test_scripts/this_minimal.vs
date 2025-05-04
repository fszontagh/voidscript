// Minimal test for this->$property comparison
class MinTest {
    private: int $value = 20;
    
    function isAdult() bool {
        // Fixed: use variable to store property first
        int $age = this->$value;
        return $age >= 18;
    }
}

// Create instance
MinTest $t = new MinTest();

// Test comparison
printnl("Is adult: ", $t->isAdult()); 