class TestClass {
    private:
    int $age = 25;
    
    public:
    function getAge() int {
        return $this->age;
    }
}

TestClass $obj = new TestClass();
print("Testing private property access from outside class:");
print($obj->age);  // This should show proper access control error