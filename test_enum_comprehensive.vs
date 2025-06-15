// Comprehensive test for enum-typed variable declarations

enum Status {
    PENDING,
    RUNNING = 10,
    DONE
};

// Test 1: Uninitialized enum variable
Status $state1;
print("Test 1: Uninitialized enum variable declared successfully");

// Test 2: Enum variable with enum value assignment
Status $state2 = Status.PENDING;
print("Test 2: Enum variable with enum value assignment works");

// Test 3: Enum variable with different enum value
Status $state3 = Status.RUNNING;
print("Test 3: Enum variable with explicit value assignment works");

// Test 4: Multiple enum variables
Status $pending_job = Status.PENDING;
Status $running_job = Status.RUNNING;
Status $done_job = Status.DONE;
print("Test 4: Multiple enum variables declared successfully");

print("All enum-typed variable declaration tests passed!");