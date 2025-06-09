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

printnl("Public property: ", $obj->$publicProperty);
printnl("Private via method: ", $obj->getPrivateProperty());
printnl("=== Valid access tests passed ===");

printnl("Attempting direct access to private property...");
string $private = $obj->$privateProperty;
printnl("ERROR: Should not reach here!");