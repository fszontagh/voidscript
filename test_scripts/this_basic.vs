// Very basic test class for $this->property syntax
class Basic {
    private: string $name = "Default";
    private: int $value = 10;
    
    function getName() string {
        return $this->name;
    }
    
    function getValue() int {
        return $this->value;
    }
}

// Create an instance
Basic $obj = new Basic();

// Test property access through methods
printnl("Name: ", $obj->getName());
printnl("Value: ", $obj->getValue()); 