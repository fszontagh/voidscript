// Working class example
class Test {
    private: string $name = "Default";
    private: int $value = 0;

    // Constructor
    function construct(string $name, int $value) {
        $this->name = $name;
        $this->value = $value;
    }

    // Getter for name
    function getName() string {
        return $this->name;
    }

    // Setter for name
    function setName(string $newName) {
        $this->name = $newName;
    }

    // Method to add to value
    function addValue(int $add) {
        $this->value = $this->value + $add;
        return $this->value;
    }
}

// Create an instance
Test $t = new Test("Test Object", 10);

// Test methods
printnl("Name: ", $t->getName());
printnl("Value after adding 5: ", $t->addValue(5));

// Change name
$t->setName("Modified Object");
printnl("New name: ", $t->getName()); 