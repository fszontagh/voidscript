class Test {
    public:
    string $name = "Test";
    
    function getName() string {
        return this->$name;
    }
}

Test $test = new Test();
printnl($test->getName()); 