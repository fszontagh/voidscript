class TestClass {
    private:
    int $age = 25;
    
    public:
    function getAge() int {
        return $this->age;
    }
    
    function debugProperties() {
        print("Debugging properties:");
        print("$this->age works: ", $this->age);
    }
}

TestClass $obj = new TestClass();
print("Testing public method access:");
$obj->debugProperties();
print("Testing getter method:");
print("Age via getter: ", $obj->getAge());
print("Now testing direct property access (should fail with proper error):");
$obj->age;