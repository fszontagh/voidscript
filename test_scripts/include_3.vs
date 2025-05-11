class Person {

    private:
    int $age = 22;
    string $name = "Person";

    function constructor(int $age, string $name) {
        this->$age = $age;
        this->$name = $name;
    }

    function getName() string {
        return this->$name;
    }

    function getAge() {
        return this->$age;
    }
}