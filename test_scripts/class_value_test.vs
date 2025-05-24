const string $var = 1;

printnl("=== CLASS VALUE TEST ===");
printnl("=== Testing class value access ===");
// Define a simple class
class TestClass {
    public:
    int $field = 42;
}
TestClass $test = new TestClass();
printnl("Field value before modification:", $test->field); // Expected: 42
$test->field = 100;
printnl("Field value after modification:", $test->field); // Expected: 100
