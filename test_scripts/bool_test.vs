// Test numeric literals in boolean expressions
int $num = 42;
bool $isPositive = $num > 0;
bool $isNegative = $num < 0;
bool $isZero = $num == 0;

printnl("num = ", $num);
printnl("isPositive: ", $isPositive);
printnl("isNegative: ", $isNegative);
printnl("isZero: ", $isZero);

// Test complex expressions with literals
bool $inRange = $num > 0 && $num < 100;
bool $isFortyTwo = $num == 42;
bool $notInRange = $num <= -1 || $num >= 1000;

printnl("inRange (0-100): ", $inRange);
printnl("isFortyTwo: ", $isFortyTwo);
printnl("notInRange: ", $notInRange);

// Class with boolean method
class BoolTest {
    private: int $value = 42;
    
    function isPositive() bool {
        printnl("Inside isPositive(), value is: ", this->$value);
        bool $result = this->$value > 0;
        printnl("Comparison result: ", $result);
        return $result;
    }
}

// Create an instance
BoolTest $obj = new BoolTest();

// Print directly
printnl("Direct method call result:");
bool $pos = $obj->isPositive();
//printnl($obj->isPositive());
