class TestAccess {
    private:
    string $private_prop = "secret";
    
    public:
    string $public_prop = "visible";
    
    function construct() {
        printnl("Test object created");
    }
}

TestAccess $obj = new TestAccess();

printnl("Testing public property access...");
printnl("Public: ", $obj->public_prop);

printnl("Testing private property access (should show access error)...");
printnl("Private: ", $obj->private_prop);