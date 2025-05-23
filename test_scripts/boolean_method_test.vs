// Simple test for boolean method call
class SimpleBoolean {
    private: int $value = 42;
    
    function isPositive() bool {
        return this->$value > 0;
    }
}

// Create an instance
SimpleBoolean $obj = new SimpleBoolean();

// Call method directly in print
printnl("Direct method call: ");
printnl($obj->isPositive());
