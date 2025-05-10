class Test {
    private:
    string $name;

    public:
    function construct(string $name) {
        this->$name = $name;
    }

    function getName() string {
        return this->$name;
    }
}

Test $test = new Test("Hello");
printnl($test->getName()); 