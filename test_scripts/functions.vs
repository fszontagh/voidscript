// Functions Feature Test

# Define a simple greeting function (no return type)
function greet (string $name) {
    const string $greeting = "Hello, ";
    printnl($greeting, $name, "!");
}

# Define a sum function with explicit return type
function sum (int $a, int $b) int {
    return $a + $b;
}

# Define a function that uses other functions and local variables
function mulSum (int $x, int $y, int $z) int {
    int $product = $x * $y;
    const int $result = sum($product, $z);
    return $result;
}

function forloopinFunct(int $from, int $to) int {
    int $c = 0;
    for (int $i = $from; $i < $to; $i++) {
        $c++;
    }
    return $c;
}

function loopOverArray(string[] $texts) int {
    int $z = 0;

    for (string $r : $tests) {
        $z++;
    }

    return $z;
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


const int $retint = forloopinFunct(0,5);
printnl("Retint: ", $retint);


string[] $arrayString = ["apple", "banana", "cherry"];
int $loopOverArrayCount = loopOverArray($arrayString);