// A complete Person class example
class Person {
    // Private properties
    private: string $name = "Unknown";
    private: int $age = 0;
    private: bool $isStudent = false;

    // Constructor
    function construct(string $name, int $age, bool $isStudent) {
        $this->name = $name;
        $this->age = $age;
        $this->isStudent = $isStudent;
    }

    // Getters
    function getName() string {
        return $this->name;
    }

    function getAge() int {
        return $this->age;
    }

    function isStudent() bool {
        return $this->isStudent;
    }

    // Setters
    function setName(string $newName) {
        $this->name = $newName;
    }

    function setAge(int $newAge) {
        $this->age = $newAge;
    }

    function setIsStudent(bool $status) {
        $this->isStudent = $status;
    }

    // Other methods
    function celebrateBirthday() int {
        $this->age++;
        return $this->age;
    }

    function canVote() bool {
        return $this->age >= 18;
    }

    function getDescription() string {
        string $status = "";
        if ($this->isStudent) {
            $status = "is";
        }else{
            $status = "is not";
        }
        return Format("{} is {} years old and {} a student", $this->name, $this->age, $status);
    }
}

// Create instances
Person $person1 = new Person("Alice", 22, true);
Person $person2 = new Person("Bob", 17, false);

// Use methods
printnl("Person 1: ", $person1->getName(), ", Age: ", $person1->getAge());
printnl("Person 2: ", $person2->getName(), ", Age: ", $person2->getAge());

// Test methods
printnl("Is Alice a student? ", $person1->isStudent());
printnl("Can Bob vote? ", $person2->canVote());

// Modify properties
printnl("Bob celebrates birthday. New age: ", $person2->celebrateBirthday());
printnl("Can Bob vote now? ", $person2->canVote());

// Change other properties
$person1->setIsStudent(false);
printnl("Is Alice a student now? ", $person1->isStudent());

// Get descriptions
printnl("Description 1: ", $person1->getDescription());
printnl("Description 2: ", $person2->getDescription());
