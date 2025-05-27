// test_scripts/enum_tests.vs

print("Starting Enum Tests...");
print(""); // Newline for readability

// 1. Basic Enum Definition and Access
print("--- Test: Basic Enum Definition and Access ---");
enum Color {
    RED,    // 0
    GREEN,  // 1
    BLUE    // 2
};

print("Color::RED: ", Color::RED);       // Expected: 0
print("Color::GREEN: ", Color::GREEN);   // Expected: 1
print("Color::BLUE: ", Color::BLUE);     // Expected: 2
print(""); // Newline

// 2. Enum with Explicit Values
print("--- Test: Enum with Explicit Values ---");
enum Status {
    PENDING = 10,
    PROCESSING,    // 11
    COMPLETED = 20,
    FAILED         // 21
};

print("Status::PENDING: ", Status::PENDING);       // Expected: 10
print("Status::PROCESSING: ", Status::PROCESSING); // Expected: 11
print("Status::COMPLETED: ", Status::COMPLETED);   // Expected: 20
print("Status::FAILED: ", Status::FAILED);         // Expected: 21
print(""); // Newline

// 3. Enum with Negative Values
print("--- Test: Enum with Negative Values ---");
enum Levels {
    LOW = -1,
    MEDIUM, // 0
    HIGH    // 1
};
print("Levels::LOW: ", Levels::LOW);         // Expected: -1
print("Levels::MEDIUM: ", Levels::MEDIUM);   // Expected: 0
print("Levels::HIGH: ", Levels::HIGH);       // Expected: 1
print(""); // Newline

// 4. Empty Enum
print("--- Test: Empty Enum Definition ---");
// Assuming empty enums are allowed by the parser
enum EmptyEnum {}; 
print("EmptyEnum defined successfully."); // Verifies parsing
print(""); // Newline

// 5. Enum members used in expressions
print("--- Test: Enum members in expressions ---");
enum Size { SMALL = 1, MEDIUM = 2, LARGE = 3 };
let s = Size::SMALL + Size::LARGE;
print("Size::SMALL + Size::LARGE: ", s); // Expected: 4
print(""); // Newline

// 6. Attempt to access non-existent member
print("--- Test: Access to non-existent enum member ---");
print("Testing access to non-existent enum member (Color::ORANGE)...");
// The following line is expected to cause a runtime error.
// The actual error message will be printed by the interpreter.
// print("Attempting to access Color::ORANGE: ", Color::ORANGE); 
// For the test script, we will print the value that would cause the error.
// The test execution framework would be responsible for catching/verifying the error.
// To make this test runnable up to the point of error, we won't execute the erroring line directly if it halts the script.
// Instead, the test is about *observing* the error when this script is run by an interpreter that supports it.
// If the interpreter halts on error, the script would stop here.
// If the language/interpreter supports try-catch for runtime errors, that would be a way to test,
// but assuming no try-catch for VoidScript based on current info.
// The line `print(Color::ORANGE);` itself would trigger the error if `Color::ORANGE` is evaluated.
// The instructions imply the script should include the line that *would* cause the error.
// So, uncommenting it for a full test.
print("Attempting to access Color::ORANGE: ", Color::ORANGE); 

print("");
print("Enum Tests Completed (or an error occurred as expected).");
