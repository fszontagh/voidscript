// Simple test class for this->$property syntax
class Simple {
    private: string $name = "Default";
    private: int $value = 10;
    
    function getName() string {
        return this->$name;
    }
    
    function setName(string $newName) {
        this->$name = $newName;
    }
    
    function getValue() int {
        return this->$value;
    }
    
    function increment() int {
        this->$value = this->$value + 1;
        return this->$value;
    }
}

// Create an instance
Simple $obj = new Simple();

// Test property access through methods
printnl("Initial name: ", $obj->getName());
printnl("Initial value: ", $obj->getValue());

// Test property modification
$obj->setName("Changed");
printnl("After setName: ", $obj->getName());

// Test property assignment with calculation
printnl("Initial value: ", $obj->getValue());
printnl("After increment: ", $obj->increment());
printnl("After increment again: ", $obj->increment()); 