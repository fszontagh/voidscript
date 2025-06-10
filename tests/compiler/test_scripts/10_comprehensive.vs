// Comprehensive test combining multiple features
// This should produce identical output in both interpreter and compiler

// Constants
const string $PROGRAM_NAME = "VoidScript Comprehensive Test";
const int $VERSION = 1;

printnl("=== ", $PROGRAM_NAME, " v", $VERSION, " ===");

// Basic variables
int $counter = 0;
string $message = "Testing";
bool $success = true;

// Function definitions
function fibonacci(int $n) int {
    if ($n <= 1) {
        return $n;
    }
    return fibonacci($n - 1) + fibonacci($n - 2);
}

function processArray(int[] $arr) {
    printnl("Processing array:");
    for (int $value : $arr) {
        printnl("  Value: ", $value, ", Doubled: ", $value * 2);
        $counter++;
    }
}

// Class definition
class TestRunner {
    private: string $name = "Default";
    private: int $tests_run = 0;
    
    function setName(string $test_name) {
        $this->name = $test_name;
    }
    
    function runTest() bool {
        $this->tests_run++;
        printnl("Running test: ", $this->name);
        return true;
    }
    
    function getTestsRun() int {
        return $this->tests_run;
    }
}

// Main execution
printnl("1. Testing basic operations");
printnl("Message: ", $message);
printnl("Success: ", $success);

printnl("\n2. Testing fibonacci function");
for (int $i = 0; $i < 6; $i++) {
    int $fib = fibonacci($i);
    printnl("fibonacci(", $i, ") = ", $fib);
}

printnl("\n3. Testing array processing");
int[] $numbers = [1, 2, 3, 4, 5];
processArray($numbers);
printnl("Total values processed: ", $counter);

printnl("\n4. Testing class operations");
TestRunner $runner = new TestRunner();
$runner->setName("Comprehensive Test");
bool $test_result = $runner->runTest();
printnl("Test result: ", $test_result);
printnl("Tests run: ", $runner->getTestsRun());

printnl("\n5. Testing object operations");
object $config = {
    string $mode : "test",
    int $timeout : 30,
    bool $verbose : true
};

printnl("Configuration:");
for (string $key, auto $value : $config) {
    printnl("  ", $key, ": ", $value);
}

printnl("\n=== Test Complete ===");