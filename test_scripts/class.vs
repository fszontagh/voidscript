class test1 {

    private:
    string $name;
    int $age;

    public:
    // constructor method, the return is always the class, can not use 'return' keyword here
    function construct(string $name, int $age) {
        this->name = $name;
        this->age = $age;
    }

    function getAge() int {
        return this->age;
    }

    function getName() string {
        return this->name;
    }

    function isAdult() bool {
        return this->age >= 18;
    }

    function changeName(string $new_name) {
        this->name = $new_name;
    }

    function incrementAge(int $incremental) int {
        return this->age + $incremental;
    }


}

//

test1 $testclass = new test1("Batman", 17);

if ($testclass->isAdult() == false) {
    printnl($testclass->getName(), " is not adult.. incrementing the age");
    $testclass->incrementAge(1);
}

if ($testclass->isAdult()) {
    println($testclass->getName(), " is adult");
}