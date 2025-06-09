// Test script to verify that bare 'this' usage is rejected
class TestClass {
    public:
        string name;
        
    function testMethod() {
        // This should cause a parse error - bare 'this' is not allowed
        this->name = "invalid";
    }
}