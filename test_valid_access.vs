// Test that valid property access still works
class TestClass {
    public:
    int $publicAge = 25;
    
    private:
    int $age = 30;
    
    public:
    function construct() {
        // Empty constructor
    }
    
    function getAge() int {
        return $this->age; // This should work - accessing private from within class
    }
}

TestClass $obj = new TestClass();
printnl("Public property access: ", $obj->publicAge);
printnl("Private property via public method: ", $obj->getAge());