class Simple {
    private:
    string $message = "Hello";

    public:
    function construct() {
        // Empty constructor
    }

    function getMessage() string {
        return this->$message;
    }
}

Simple $obj = new Simple();
printnl($obj->getMessage()); 