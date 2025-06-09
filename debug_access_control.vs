class TestClass {
    private:
    string $privateProperty = "Private Value";
    
    public:
    string $publicProperty = "Public Value";
    
    function getPrivateProperty() string {
        return this->$privateProperty;
    }
}

TestClass $obj = new TestClass();
printnl("Testing access control...");