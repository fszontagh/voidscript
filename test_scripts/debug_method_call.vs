class test1 {
    private:
    int $age = 17;

    public:
    function construct(string $name, int $age) {
        $this->age = $age;
        printnl("Constructor called with age: ", $this->age);
    }

    function getAge() int {
        printnl("getAge() called, $this->age = ", $this->age);
        return $this->age;
    }

    function incrementAge2() {
        printnl("incrementAge2() called, before: $this->age = ", $this->age);
        $this->age = $this->age + 1;
        printnl("incrementAge2() called, after: $this->age = ", $this->age);
    }
}

test1 $testclass2 = new test1("Batman", 33);

printnl("Before incrementAge2():");
printnl("Age via getAge(): ", $testclass2->getAge());

printnl("Calling incrementAge2()...");
$testclass2->incrementAge2();

printnl("After incrementAge2():");
printnl("Age via getAge(): ", $testclass2->getAge());