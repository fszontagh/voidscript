// Extremely minimal class example
class Simple {
    public: int $value = 42;
    
    // Add a simple method with no return
    function printValue() {
        printnl("Inside class method - value is: ", $this->value);
    }
}

// Create an instance
Simple $obj = new Simple();

// Access the public property
printnl("Value: ", $obj->value);

// Call the method
$obj->printValue(); 