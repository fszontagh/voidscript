// Simple test for this->$property with comparisons
class AgeChecker {
    private: int $age = 20;
    
    function setAge(int $newAge) null {
        this->$age = $newAge;
    }
    
    function getAge() int {
        return this->$age;
    }
    
    function isAdult() bool {
        // Test comparison with property
        return this->$age >= 18;
    }
    
    function getCategory() string {
        if (this->$age < 13) {
            return "Child";
        } else if (this->$age < 18) {
            return "Teen";
        } else if (this->$age < 65) {
            return "Adult";
        } else {
            return "Senior";
        }
    }
}

// Create an instance
AgeChecker $person = new AgeChecker();

// Test with default age (20)
printnl("Age: ", $person->getAge());
printnl("Is adult: ", $person->isAdult());
printnl("Category: ", $person->getCategory());

// Test with child age
$person->setAge(10);
printnl("Age: ", $person->getAge());
printnl("Is adult: ", $person->isAdult());
printnl("Category: ", $person->getCategory());

// Test with teen age
$person->setAge(15);
printnl("Age: ", $person->getAge());
printnl("Is adult: ", $person->isAdult());
printnl("Category: ", $person->getCategory());

// Test with senior age
$person->setAge(70);
printnl("Age: ", $person->getAge());
printnl("Is adult: ", $person->isAdult());
printnl("Category: ", $person->getCategory()); 