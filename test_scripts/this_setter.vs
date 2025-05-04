// Basic test class for this->$property setter
class Setter {
    private: string $name = "Default";
    
    function getName() string {
        return this->$name;
    }
    
    function setName(string $newName) {
        this->$name = $newName;
    }
}

// Create an instance
Setter $obj = new Setter();

// Test initial property
printnl("Initial name: ", $obj->getName());

// Test setter method
$obj->setName("Changed");
printnl("After setName: ", $obj->getName()); 