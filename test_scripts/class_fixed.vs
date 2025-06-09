// class name can not be: function, string, null, int, etc...
class test1 {

    // private methods and variables can not be accessible outside from the class
    private:
    string $name = "No Name"; // pre defined variables allowed
    int $age = 17;
    // const variables allowed too
    const string $test = "This is a test";

    // private method only inner usage
    function WhatIsTheAge() int {
        return $this->age;
    }

    public:
    string $public_str = "Public str";
    // constructor method, the return type and value is always the class itself to make it callable, can not use 'return' keyword here
    // constructor is optional
    function construct(string $name, int $age) {
        $this->name = $name;
        $this->age = $age;
        printnl("Constructor called with name: ", $this->name, " and age: ", $this->age);
        // return here drops error
    }

    // other public methods
    function getAge() int {
        return $this->age;
    }

    function getName() string {
        return $this->name;
    }

    function isAdult() bool {
        return $this->age >= 18;
    }

    function changeName(string $new_name) {
        $this->name = $new_name;
    }

    function setAge(int $newAge) null {
        $this->age = $newAge;
    }

    function incrementAge(int $incremental) int {
        $this->age += $incremental; // Modify the age
        return $this->age; // Return the *new* age
    }
    function incrementAge2() {
        $this->age = $this->age + 1;
    }
}


function getInt() int {
    return 10;
}
test1 $testclass = new test1("John Doe", 11);
test1 $testclass2 = new test1("Batman", 33);

// Fixed: Call incrementAge2() without parameters as it's defined
$testclass2->incrementAge2();
int $age = $testclass2->getAge();
printnl("incremented age: ", $testclass2->getAge());


printnl("Publicstr: ", $testclass2->public_str);
printnl("incremented age: ", $testclass2->getAge());