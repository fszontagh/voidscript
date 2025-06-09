class test1 {
    private:
    int $age = 17;

    public:
    function construct(string $name, int $age) {
        printnl("construct() - setting age to: ", $age);
        $this->age = $age;
        printnl("construct() - $this->age is now: ", $this->age);
    }

    function getAge() int {
        printnl("getAge() - ENTRY");
        printnl("getAge() - trying to access $this->age...");
        return $this->age;
    }

    function testThis() {
        printnl("testThis() - ENTRY");
        printnl("testThis() - trying to access $this->age...");
        printnl("testThis() - $this->age = ", $this->age);
    }
}

printnl("Creating object...");
test1 $testclass2 = new test1("Batman", 33);

printnl("Calling testThis()...");
$testclass2->testThis();

printnl("Calling getAge()...");
int $result = $testclass2->getAge();
printnl("getAge() returned: ", $result);