// Simple class example
class Person {
    private: string $name;
    private: int $age;

    // Constructor is not needed as we can set properties directly

    // Getter for name
    function getName() string {
        return this->$name;
    }

    // Getter for age
    function getAge() int {
        return this->$age;
    }

    // Setter for name
    function setName(string $newName) null {
        this->$name = $newName;
    }

    // Setter for age
    function setAge(int $newAge) null {
        this->$age = $newAge;
    }

    // Check if person is adult
    function isAdult() bool {
        return this->$age >= 18;
    }
}

// Create an instance
Person $person = new Person();

// Set properties
$person->setName("John");
$person->setAge(25);

// Get properties
printnl("Name: ", $person->getName());
printnl("Age: ", $person->getAge());

// Use a method
if ($person->isAdult()) {
    printnl($person->getName(), " is an adult");
} else {
    printnl($person->getName(), " is not an adult");
} 