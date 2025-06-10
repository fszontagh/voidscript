// Test constants and immutable values
const int $MAX_SIZE = 100;
const string $APP_NAME = "VoidScript Compiler Test";
const bool $DEBUG_MODE = true;
const float $PI = 3.14159;

printnl("Max size: ", $MAX_SIZE);
printnl("App name: ", $APP_NAME);
printnl("Debug mode: ", $DEBUG_MODE);
printnl("Pi value: ", $PI);

// Using constants in calculations
function calculateCircleArea(float $radius) float {
    return $PI * $radius * $radius;
}

float $area = calculateCircleArea(5.0);
printnl("Circle area (radius 5): ", $area);

// Constants in conditions
if ($DEBUG_MODE) {
    printnl("Debug information enabled");
} else {
    printnl("Debug information disabled");
}

// Using constants as array bounds
int[] $buffer = [];
for (int $i = 0; $i < 5; $i++) {
    $buffer[$i] = $i * 2;
}

printnl("Buffer contents:");
for (int $value : $buffer) {
    printnl("  ", $value);
}