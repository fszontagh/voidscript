// Comprehensive test for this->$property syntax
class Person {
    private: string $name;
    private: int $age;
    private: bool $isStudent = false;
    
    // Constructor
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
    }
    
    // Simple getter
    function getName() string {
        return this->$name;
    }
    
    // Simple getter
    function getAge() int {
        return this->$age;
    }
    
    // Conditional getter
    function isStudent() bool {
        return this->$isStudent;
    }
    
    // Setter with return
    function setName(string $newName) string {
        this->$name = $newName;
        return this->$name;
    }
    
    // Setter with return
    function setAge(int $newAge) int {
        this->$age = $newAge;
        return this->$age;
    }
    
    // Toggle method that modifies property
    function toggleStudentStatus() bool {
        this->$isStudent = !this->$isStudent;
        return this->$isStudent;
    }
    
    // Method with property calculation
    function haveBirthday() int {
        this->$age = this->$age + 1;
        return this->$age;
    }
    
    // Method with conditional based on property
    function isAdult() bool {
        return this->$age >= 18;
    }
    
    // Method with string interpolation using properties
    function getInfo() string {
        string $status = this->$isStudent ? "is" : "is not";
        return this->$name + " is " + this->$age + " years old and " + $status + " a student.";
    }
}

// Create a person instance
Person $alice = new Person("Alice", 25);

// Test initial values
printnl("Name: ", $alice->getName());
printnl("Age: ", $alice->getAge());
printnl("Is student: ", $alice->isStudent());

// Test setters
printnl("Set name: ", $alice->setName("Alicia"));
printnl("Set age: ", $alice->setAge(26));

// Test toggle method
printnl("Toggle student status: ", $alice->toggleStudentStatus());
printnl("Toggle student status again: ", $alice->toggleStudentStatus());

// Test birthday method
printnl("After birthday: ", $alice->haveBirthday());

// Test conditional method
printnl("Is adult: ", $alice->isAdult());

// Test info method
printnl("Info: ", $alice->getInfo());

// Test combined methods
$alice->setName("Alexandra");
$alice->setAge(17);
$alice->toggleStudentStatus();
printnl("Updated info: ", $alice->getInfo());
printnl("Is adult: ", $alice->isAdult()); 