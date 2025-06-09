// Best practice for $this->property comparison
class BestPractice {
    private: int $value = 20;
    private: int $min = 10;
    private: int $max = 30;
    
    // Store property in local variable for comparisons
    function isInRange() bool {
        int $val = $this->value;
        int $minVal = $this->min;
        int $maxVal = $this->max;
        
        return $val >= $minVal && $val <= $maxVal;
    }
    
    // Update a property using other properties
    function incrementValue(int $amount) int {
        int $current = $this->value;
        int $newValue = $current + $amount;
        $this->value = $newValue;
        return $this->value;
    }
}

// Create instance
BestPractice $test = new BestPractice();

// Test comparison
printnl("Is in range: ", $test->isInRange());

// Test property update
printnl("Increment by 5: ", $test->incrementValue(5));
printnl("Increment by 10: ", $test->incrementValue(10)); 