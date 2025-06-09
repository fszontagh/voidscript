// Final test class for this->property syntax
class Final {
    private: string $name = "Default";
    
    function getName() string {
        return $this->name;
    }
    
    function setName(string $newName) string {
        $this->name = $newName;
        return $this->name;  // Return the updated value
    }
}

// Create an instance
Final $obj = new Final();

// Test initial property
printnl("Initial name: ", $obj->getName());

// Test setter method
printnl("After setName: ", $obj->setName("Changed")); 