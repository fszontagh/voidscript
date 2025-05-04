class SimpleTest {
    public: string $name = "Test";
    public: int $value = 42;
}

// Create an instance
SimpleTest $test = new SimpleTest();

// Access public properties
printnl("Name: ", $test->name);
printnl("Value: ", $test->value); 