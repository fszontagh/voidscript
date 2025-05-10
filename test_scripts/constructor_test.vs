class Person {
    private:
    string $name;
    int $age;

    public:
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
    }

    function getName() string {
        return this->$name;
    }

    function getAge() int {
        return this->$age;
    }
}

Person $person = new Person("John", 25);
printnl("Name: ", $person->getName());
printnl("Age: ", $person->getAge()); 