class TestClass {
    private:
    string $name = "private_value";
    
    public:
    function construct() {
        printnl("Constructor called");
    }
}

TestClass $obj = new TestClass();
printnl("About to access private property...");
printnl("Value: ", $obj->name);
printnl("This should not be reached");