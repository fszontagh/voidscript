// This is a minimal class example using the $this->variable syntax
class Test {
    private: 
    string $name;
    int $value;

    public:
    // Constructor
    function construct(string $name, int $value) {
        $this->name = $name;
        $this->value = $value;
    }

    // Simple method
    function getName() string {
        return $this->name;
    }

    // Method with parameters
    function addToValue(int $amount) int {
        return $this->value + $amount;
    }
}

// Create an instance
Test $test = new Test("Example", 42);

// Print the name
printnl("Name: ", $test->getName());

// Test method with parameters
printnl("Value + 10: ", $test->addToValue(10)); 