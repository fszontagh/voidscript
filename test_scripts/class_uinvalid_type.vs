class Test {
    string $name = "NoName";
    int $age = 11;

    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
    }


    function getAge() int {
        return this->$age;
    }

    function getName() string {
        return this->$name;
    }

    function incrementAge(int $num) {

    }

}

Test $t1 = new Test();
//$t1->incrementAge(1);

//printnl("Age: ", $t1->getAge());

//Test $t2 = new Test("Jane", 25);

//printnl("t2 name = ", $t2->getName(), " age: ", $t2->getAge());
printnl("t1 name = ", $t1->getName(), " age: ", $t1->getAge());