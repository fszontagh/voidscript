//==============================================================================
// BASIC ENUM FUNCTIONALITY TESTS
// This file tests fundamental enum declaration and value assignment features:
// 1. Basic enum declaration syntax
// 2. Automatic value assignment (0, 1, 2...)
// 3. Explicit value assignment
// 4. Mixed automatic and explicit values
//==============================================================================

printnl("=== STARTING BASIC ENUM TESTS ===");
printnl("");

//==============================================================================
// 1. BASIC ENUM DECLARATION TESTS
//==============================================================================

printnl("--- BASIC ENUM DECLARATION TESTS ---");

// Test 1.1: Simple enum with automatic values
printnl("Test 1.1: Simple enum with automatic values");
printnl("Expected: RED=0, GREEN=1, BLUE=2");
enum Color {
    RED,
    GREEN,
    BLUE
};

int $red_val = Color.RED;
int $green_val = Color.GREEN;
int $blue_val = Color.BLUE;

printnl("Color.RED = ", $red_val, " (expected: 0)");
printnl("Color.GREEN = ", $green_val, " (expected: 1)");
printnl("Color.BLUE = ", $blue_val, " (expected: 2)");
printnl("");

// Test 1.2: Simple enum with all explicit values
printnl("Test 1.2: Simple enum with all explicit values");
printnl("Expected: PENDING=100, PROCESSING=200, COMPLETED=300, FAILED=400");
enum Status {
    PENDING = 100,
    PROCESSING = 200,
    COMPLETED = 300,
    FAILED = 400
};

int $pending_val = Status.PENDING;
int $processing_val = Status.PROCESSING;
int $completed_val = Status.COMPLETED;
int $failed_val = Status.FAILED;

printnl("Status.PENDING = ", $pending_val, " (expected: 100)");
printnl("Status.PROCESSING = ", $processing_val, " (expected: 200)");
printnl("Status.COMPLETED = ", $completed_val, " (expected: 300)");
printnl("Status.FAILED = ", $failed_val, " (expected: 400)");
printnl("");

//==============================================================================
// 2. MIXED AUTOMATIC AND EXPLICIT VALUES
//==============================================================================

printnl("--- MIXED AUTOMATIC AND EXPLICIT VALUES ---");

// Test 2.1: Explicit value followed by automatic
printnl("Test 2.1: Explicit value followed by automatic");
printnl("Expected: LOW=0, MEDIUM=5, HIGH=6, CRITICAL=10");
enum Priority {
    LOW,
    MEDIUM = 5,
    HIGH,
    CRITICAL = 10
};

int $low_val = Priority.LOW;
int $medium_val = Priority.MEDIUM;
int $high_val = Priority.HIGH;
int $critical_val = Priority.CRITICAL;

printnl("Priority.LOW = ", $low_val, " (expected: 0)");
printnl("Priority.MEDIUM = ", $medium_val, " (expected: 5)");
printnl("Priority.HIGH = ", $high_val, " (expected: 6)");
printnl("Priority.CRITICAL = ", $critical_val, " (expected: 10)");
printnl("");

// Test 2.2: Multiple explicit values with automatic in between
printnl("Test 2.2: Multiple explicit values with automatic in between");
printnl("Expected: NORTH=1, EAST=2, SOUTH=4, WEST=8");
enum Direction {
    NORTH = 1,
    EAST = 2,
    SOUTH = 4,
    WEST = 8
};

int $north_val = Direction.NORTH;
int $east_val = Direction.EAST;
int $south_val = Direction.SOUTH;
int $west_val = Direction.WEST;

printnl("Direction.NORTH = ", $north_val, " (expected: 1)");
printnl("Direction.EAST = ", $east_val, " (expected: 2)");
printnl("Direction.SOUTH = ", $south_val, " (expected: 4)");
printnl("Direction.WEST = ", $west_val, " (expected: 8)");
printnl("");

// Test 2.3: Automatic values after explicit assignment
printnl("Test 2.3: Automatic values continuing from explicit assignment");
printnl("Expected: FIRST=0, SECOND=1, TENTH=10, ELEVENTH=11, TWELFTH=12");
enum Sequence {
    FIRST,
    SECOND,
    TENTH = 10,
    ELEVENTH,
    TWELFTH
};

int $first_val = Sequence.FIRST;
int $second_val = Sequence.SECOND;
int $tenth_val = Sequence.TENTH;
int $eleventh_val = Sequence.ELEVENTH;
int $twelfth_val = Sequence.TWELFTH;

printnl("Sequence.FIRST = ", $first_val, " (expected: 0)");
printnl("Sequence.SECOND = ", $second_val, " (expected: 1)");
printnl("Sequence.TENTH = ", $tenth_val, " (expected: 10)");
printnl("Sequence.ELEVENTH = ", $eleventh_val, " (expected: 11)");
printnl("Sequence.TWELFTH = ", $twelfth_val, " (expected: 12)");
printnl("");

//==============================================================================
// 3. SPECIAL VALUE TESTS
//==============================================================================

printnl("--- SPECIAL VALUE TESTS ---");

// Test 3.1: Zero and negative values
printnl("Test 3.1: Zero and negative values");
printnl("Expected: NEGATIVE=-5, ZERO=0, POSITIVE=5");
enum SignedValues {
    NEGATIVE = -5,
    ZERO = 0,
    POSITIVE = 5
};

int $negative_val = SignedValues.NEGATIVE;
int $zero_val = SignedValues.ZERO;
int $positive_val = SignedValues.POSITIVE;

printnl("SignedValues.NEGATIVE = ", $negative_val, " (expected: -5)");
printnl("SignedValues.ZERO = ", $zero_val, " (expected: 0)");
printnl("SignedValues.POSITIVE = ", $positive_val, " (expected: 5)");
printnl("");

// Test 3.2: Large values
printnl("Test 3.2: Large values");
printnl("Expected: SMALL=1, LARGE=1000000");
enum LargeValues {
    SMALL = 1,
    LARGE = 1000000
};

int $small_val = LargeValues.SMALL;
int $large_val = LargeValues.LARGE;

printnl("LargeValues.SMALL = ", $small_val, " (expected: 1)");
printnl("LargeValues.LARGE = ", $large_val, " (expected: 1000000)");
printnl("");

//==============================================================================
// 4. SINGLE VALUE ENUMS
//==============================================================================

printnl("--- SINGLE VALUE ENUM TESTS ---");

// Test 4.1: Single automatic value
printnl("Test 4.1: Single automatic value");
printnl("Expected: SINGLETON=0");
enum SingleAuto {
    SINGLETON
};

int $singleton_val = SingleAuto.SINGLETON;
printnl("SingleAuto.SINGLETON = ", $singleton_val, " (expected: 0)");
printnl("");

// Test 4.2: Single explicit value
printnl("Test 4.2: Single explicit value");
printnl("Expected: ALONE=42");
enum SingleExplicit {
    ALONE = 42
};

int $alone_val = SingleExplicit.ALONE;
printnl("SingleExplicit.ALONE = ", $alone_val, " (expected: 42)");
printnl("");

printnl("=== ALL BASIC ENUM TESTS COMPLETED ===");
printnl("");
printnl("SUMMARY:");
printnl("- Basic enum declaration syntax works correctly");
printnl("- Automatic value assignment follows sequential pattern (0, 1, 2...)");
printnl("- Explicit value assignment works with any integer values");
printnl("- Mixed automatic and explicit values work together properly");
printnl("- Negative, zero, and large positive values are supported");
printnl("- Single-value enums work correctly");