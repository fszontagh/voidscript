class Test {
    string $name = "NoName";
    int $age = 11;

    function construct(string $name_, int $age_) {
        $this->age = $age_;
        $this->name = $name_;
        printnl("Test::construct called");
    }

    function setName(string $newName) {
        printnl("Method: setName: $this->name: ", $this->name, " $newName: ", $newName);
        $this->name = $newName;
        printnl("Method: setName: $this->name: ", $this->name, " $newName: ", $newName);
    }

    function getAge() int {
        return $this->age;
    }

    function getName() string {
        return $this->name;
    }

    function incrementAge(int $num) {

    }

}

Test $t1 = new Test("Batman", 21);
$t1->setName("New name");
//$t1->incrementAge(1);

//printnl("Age: ", $t1->getAge());

//Test $t2 = new Test("Jane", 25);

//printnl("t2 name = ", $t2->getName(), " age: ", $t2->getAge());
printnl("t1 name = ", $t1->getName(), " age: ", $t1->getAge());