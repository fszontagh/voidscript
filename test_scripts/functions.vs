// Functions Feature Test

# Define a simple greeting function (no return type)
function greet (string $name) {
    printnl("Hello, ", $name, "!");
}

# Define a sum function with explicit return type
function sum (int $a, int $b) int {
    return $a + $b;
}

# Define a function that uses other functions and local variables
function mulSum (int $x, int $y, int $z) int {
    int $product = $x * $y;
    int $result = sum($product, $z);
    return $result;
}

# Call the functions and print results
greet("VoidScript");

int $result1 = sum(7, 5);
printnl("sum(7, 5) = ", $result1);

int $result2 = mulSum(2, 3, 4);
printnl("mulSum(2, 3, 4) = ", $result2);

# Demonstrate nested call in expression
int $combined = mulSum(1, 2, 3) + sum(3, 4);
printnl("combined (mulSum(1,2,3) + sum(3,4)) = ", $combined);