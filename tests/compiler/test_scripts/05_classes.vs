// Test basic class definitions and usage
class Calculator {
    private: int $value = 0;
    
    function setValue(int $val) {
        $this->value = $val;
    }
    
    function getValue() int {
        return $this->value;
    }
    
    function add(int $num) int {
        $this->value = $this->value + $num;
        return $this->value;
    }
    
    function multiply(int $num) int {
        $this->value = $this->value * $num;
        return $this->value;
    }
}

// Create instance and test methods
Calculator $calc = new Calculator();
$calc->setValue(10);

printnl("Initial value: ", $calc->getValue());

int $result1 = $calc->add(5);
printnl("After adding 5: ", $result1);

int $result2 = $calc->multiply(2);
printnl("After multiplying by 2: ", $result2);

printnl("Final value: ", $calc->getValue());