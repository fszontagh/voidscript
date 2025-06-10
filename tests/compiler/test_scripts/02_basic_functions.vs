// Test basic function definitions and calls
function greet(string $name) {
    printnl("Hello, ", $name, "!");
}

function add(int $a, int $b) int {
    return $a + $b;
}

function multiply(int $x, int $y) int {
    int $result = $x * $y;
    return $result;
}

// Function calls
greet("VoidScript");
int $sum = add(5, 3);
printnl("Sum: ", $sum);

int $product = multiply(4, 7);
printnl("Product: ", $product);

// Nested function calls
int $nested = add(multiply(2, 3), add(1, 2));
printnl("Nested result: ", $nested);