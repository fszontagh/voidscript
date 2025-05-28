// test_scripts/switch_tests.vs
print("Starting Switch Tests...");
print(""); // Newline for readability

// 1. Basic Switch with Enum and break
print("--- Test 1: Basic Switch with Enum and break ---");
enum Signal {
    RED,    // 0
    YELLOW, // 1
    GREEN   // 2
};

enum $currentSignal = Signal::YELLOW; // MODIFIED
print("Testing Signal: YELLOW (currentSignal value: ", $currentSignal, ")"); // MODIFIED
switch ($currentSignal) { // MODIFIED
    case Signal::RED:
        print("Stop!");
        break;
    case Signal::YELLOW:
        print("Caution!"); // Expected
        break;
    case Signal::GREEN:
        print("Go!");
        break;
    default:
        print("Unknown signal.");
        break;
}; // MODIFIED: Added semicolon
print(""); // Newline

// 2. Switch with Fall-through
print("--- Test 2: Switch with Fall-through ---");
$currentSignal = Signal::RED; // MODIFIED
print("Testing Signal: RED (with fall-through to YELLOW, currentSignal value: ", $currentSignal, ")"); // MODIFIED
switch ($currentSignal) { // MODIFIED
    case Signal::RED:
        print("Stop!"); // Expected
    case Signal::YELLOW: // Fall-through
        print("Caution!"); // Expected
        break;
    case Signal::GREEN:
        print("Go!");
        break;
    default:
        print("Unknown signal.");
}; // MODIFIED: Added semicolon
print(""); // Newline

// 3. Switch with default case
print("--- Test 3: Switch with default case ---");
let someValue = 100;
print("Testing someValue: 100 (default case)");
switch (someValue) {
    case 0:
        print("Value is 0");
        break;
    case 1:
        print("Value is 1");
        break;
    default:
        print("Value is something else: ", someValue); // Expected
        break;
}; // MODIFIED: Added semicolon
print(""); // Newline

// 4. Switch with no matching case and no default
print("--- Test 4: Switch with no matching case and no default ---");
print("Testing value: 1 (no matching case, no default)");
switch (1) { // Use a literal directly
    case 10:
        print("Value is 10");
        break;
    case 20:
        print("Value is 20");
        break;
}; // MODIFIED: Added semicolon
// Expected: No output from this switch block itself
print("After switch with no matching case and no default.");
print(""); // Newline

// 5. Switch on an integer directly
print("--- Test 5: Switch on an integer directly ---");
let num = 2;
print("Testing num: 2");
switch (num) {
    case 1: print("One"); break;
    case 2: print("Two"); break; // Expected
    case 3: print("Three"); break;
    default: print("Other"); break;
}; // MODIFIED: Added semicolon
print(""); // Newline

// 6. Switch with expression in case
print("--- Test 6: Switch with expression in case ---");
enum Vals { A = 1, B = 2, C = 3};
let testVal = 3;
print("Testing testVal: 3 (case with expression Vals::A + Vals::B)");
switch (testVal) {
    case Vals::A + Vals::B: // 1 + 2 = 3
        print("Case A+B matched!"); // Expected
        break;
    default:
        print("Default for A+B test");
        break;
}; // MODIFIED: Added semicolon
print(""); // Newline

// 7. Switch with expression in switch argument
print("--- Test 7: Switch with expression in switch argument ---");
print("Testing switch (Vals::A * Vals::C)"); // 1 * 3 = 3
switch (Vals::A * Vals::C) {
    case Vals::C: 
        print("Case C matched!"); // Expected
        break;
    default:
        print("Default for A*C test");
        break;
}; // MODIFIED: Added semicolon
print(""); // Newline

// 8. Test error: Switch on non-integer
print("--- Test 8: Switch on a string (should error) ---");
print("The next operation is expected to cause a runtime error if executed.");
// The actual error message will be printed by the interpreter.
// This line itself, if the interpreter processes it, should halt or throw an error.
switch ("hello") { 
    case "hello": // This case value will also be an error if only integers are allowed
        print("Matched string hello");
        break;
    default:
        print("Default for string switch");
        break;
}; // MODIFIED: Added semicolon

print("");
print("Switch Tests Completed (or an error occurred as expected).");
