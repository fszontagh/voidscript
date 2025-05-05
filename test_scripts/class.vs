// class name can not be: function, string, null, int, etc...
class test1 {

    // private methods and variables can not be accessible outside from the class
    private:
    string $name = "No Name"; // pre defined variables allowed
    int $age;
    // const variables allowed too
    const string $test = "This is a test";
    // private method only inner usage
    function WhatIsTheAge() int {
        return this->$age;
    }

    public:
    // constructor method, the return type and value is always the class itself to make it callable, can not use 'return' keyword here
    // constructor is optional
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
        // return here drops error
    }

    // other public methods
    function getAge() int {
        return this->$age;
    }

    function getName() string {
        return this->$name;
    }

    function isAdult() bool {
        return this->$age >= 18;
    }

    function changeName(string $new_name) {
        this->$name = $new_name;
    }

    function incrementAge(int $incremental) int {
        this->$age += $incremental; // Modify the age
        return this->$age; // Return the *new* age
    }
}


function getInt() int {
    return 10;
}
// create an instance of the class, where the $testClass variable type is the class name, adding parameters based on the constructor
test1 $testclass = new test1("Batman", 17);

int $z = getInt();

printnl("Z: ", $z);

if ($testclass->isAdult() == false) {
    printnl($testclass->getName(), " is not adult.. incrementing the age");
    int $new_age = $testclass->incrementAge(1);
    printnl("New age would be: ", $testclass->incrementAge(1));
}

if ($testclass->isAdult()) {
    printnl($testclass->getName(), " is adult ", $testclass->getAge());
} else {
    printnl($testclass->getName(), " is not adult ", $testclass->getAge());
}