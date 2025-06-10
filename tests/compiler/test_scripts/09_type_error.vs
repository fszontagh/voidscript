// Test file with type errors - should fail compilation
int $number = 42;
string $text = "hello";

// Type mismatch: assigning string to int
$number = "not a number";

// Type mismatch in function call
function requiresInt(int $param) {
    printnl("Value: ", $param);
}

requiresInt($text);  // Passing string where int expected

// Type mismatch in return
function shouldReturnInt() int {
    return "string instead of int";
}

int $result = shouldReturnInt();
printnl("Result: ", $result);