// Basic test for class method call
class SimpleTest {
    private: int $value = 42;
    
    function getValue() int {
        return $this->value;
    }
    
    function isPositive() bool {
        return $this->value > 0;
    }
}

SimpleTest $obj = new SimpleTest();
printnl("Calling $obj->getValue() directly:");
printnl($obj->getValue());

printnl("Calling $obj->isPositive() directly:");
printnl($obj->isPositive());

// Now test with variable assignment
int $result = $obj->getValue();
printnl("The value is: ", $result);

bool $isPos = $obj->isPositive();
printnl("Is positive: ", $isPos);
