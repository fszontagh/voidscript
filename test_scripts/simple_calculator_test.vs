// Testing Calculator step by step
class SimpleCalculator {
    int $number = 42;
    
    function construct() {
        printnl("Constructor called");
    }
    
    function getNumber() int {
        return this->$number;
    }
}

SimpleCalculator $calc = new SimpleCalculator();
printnl("Number: ", $calc->getNumber());
