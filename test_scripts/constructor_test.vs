// Test script for constructor implementation

class Person {
    public: string $name = "";
    public: int $age = 0;

    // Proper constructor with parameters
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
        print("Person created with name: " + this->$name + " and age: " + this->$age + "\n");
    }
    
    // Simple info method
    function getInfo() {
        return "Name: " + this->$name + ", Age: " + this->$age;
    }

    // Setter methods
    function setName(string $new_name) {
        this->$name = $new_name;
    }

    function setAge(int $new_age) {
        this->$age = $new_age;
    }
}


// Create instance with constructor
Person $person = new Person("John Doe", 43);
print("Person info: " + $person->getInfo() + "\n");


// Test parameterless constructor
Person $person2 = new Person();
print("Person 2 info: " + $person2->getInfo() + "\n");

print("Constructor test completed successfully!\n");
