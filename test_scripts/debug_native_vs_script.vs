class test1 {
    public:
    int $age = 33;

    function getAge() int {
        printnl("getAge() - SCRIPT METHOD CALLED");
        return $this->age;
    }
}

test1 $obj = new test1();
printnl("Calling getAge()...");
int $result = $obj->getAge();
printnl("Result: ", $result);