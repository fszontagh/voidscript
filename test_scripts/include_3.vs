// Included by include.vs. Provides the Person class.
class Person {
    private:
        string $name = "";
        int $age = 0;
    public:
        function construct(string $name, int $age) {
            $this->name = $name;
            $this->age = $age;
        }
        function getName() string { return $this->name; }
        function getAge() int { return $this->age; }
}
