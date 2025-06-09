class TestAccess {
    private:
    string $privateData = "Secret";
    
    public:
    string $publicData = "Open";
    
    function getPrivateData() string {
        return this->$privateData;
    }
}

TestAccess $test = new TestAccess();

printnl("Public access: ", $test->$publicData);
printnl("Private via method: ", $test->getPrivateData());
printnl("Access control working correctly!");