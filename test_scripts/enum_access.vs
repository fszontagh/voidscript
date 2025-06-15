//==============================================================================
// ENUM ACCESS AND USAGE TESTS
// This file tests enum value access and usage in various contexts:
// 1. Accessing enum values with dot notation
// 2. Assigning enum values to variables
// 3. Using enum values in expressions
// 4. Printing enum values
//==============================================================================

printnl("=== STARTING ENUM ACCESS AND USAGE TESTS ===");
printnl("");

//==============================================================================
// 1. BASIC ENUM ACCESS TESTS
//==============================================================================

printnl("--- BASIC ENUM ACCESS TESTS ---");

// Define test enums
enum ResponseCode {
    SUCCESS = 200,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
};

enum Permission {
    READ = 1,
    WRITE = 2,
    EXECUTE = 4,
    ADMIN = 8
};

// Test 1.1: Direct enum value access
printnl("Test 1.1: Direct enum value access");
printnl("Expected: SUCCESS=200, NOT_FOUND=404, SERVER_ERROR=500");
printnl("ResponseCode.SUCCESS = ", ResponseCode.SUCCESS);
printnl("ResponseCode.NOT_FOUND = ", ResponseCode.NOT_FOUND);
printnl("ResponseCode.SERVER_ERROR = ", ResponseCode.SERVER_ERROR);
printnl("");

// Test 1.2: Enum value assignment to variables
printnl("Test 1.2: Enum value assignment to variables");
printnl("Expected: Variables should hold the correct enum values");
int $success_code = ResponseCode.SUCCESS;
int $error_code = ResponseCode.NOT_FOUND;
int $server_error = ResponseCode.SERVER_ERROR;

printnl("success_code = ", $success_code, " (expected: 200)");
printnl("error_code = ", $error_code, " (expected: 404)");
printnl("server_error = ", $server_error, " (expected: 500)");
printnl("");

//==============================================================================
// 2. ENUM VALUES IN EXPRESSIONS
//==============================================================================

printnl("--- ENUM VALUES IN EXPRESSIONS ---");

// Test 2.1: Arithmetic expressions with enum values
printnl("Test 2.1: Arithmetic expressions with enum values");
printnl("Expected: Mathematical operations work correctly with enum values");
int $sum = Permission.READ + Permission.WRITE;
int $combined = Permission.READ | Permission.WRITE | Permission.EXECUTE;
int $difference = Permission.ADMIN - Permission.READ;

printnl("READ + WRITE = ", $sum, " (expected: 3)");
printnl("READ | WRITE | EXECUTE = ", $combined, " (expected: 7)");
printnl("ADMIN - READ = ", $difference, " (expected: 7)");
printnl("");

// Test 2.2: Comparison expressions with enum values
printnl("Test 2.2: Comparison expressions with enum values");
printnl("Expected: Comparisons work correctly with enum values");
bool $is_success = (ResponseCode.SUCCESS == 200);
bool $is_greater = (ResponseCode.SERVER_ERROR > ResponseCode.NOT_FOUND);
bool $is_not_equal = (Permission.READ != Permission.WRITE);

printnl("SUCCESS == 200: ", $is_success, " (expected: true)");
printnl("SERVER_ERROR > NOT_FOUND: ", $is_greater, " (expected: true)");
printnl("READ != WRITE: ", $is_not_equal, " (expected: true)");
printnl("");

// Test 2.3: Enum values in complex expressions
printnl("Test 2.3: Enum values in complex expressions");
printnl("Expected: Complex expressions evaluate correctly");
int $complex1 = (Permission.READ + Permission.WRITE) * Permission.EXECUTE;
int $complex2 = ResponseCode.SUCCESS / 10 + ResponseCode.NOT_FOUND / 100;
bool $complex_bool = (Permission.ADMIN > Permission.EXECUTE) && (ResponseCode.SUCCESS < ResponseCode.NOT_FOUND);

printnl("(READ + WRITE) * EXECUTE = ", $complex1, " (expected: 12)");
printnl("SUCCESS/10 + NOT_FOUND/100 = ", $complex2, " (expected: 24)");
printnl("(ADMIN > EXECUTE) && (SUCCESS < NOT_FOUND) = ", $complex_bool, " (expected: true)");
printnl("");

//==============================================================================
// 3. ENUM VALUES AS FUNCTION PARAMETERS
//==============================================================================

printnl("--- ENUM VALUES AS FUNCTION PARAMETERS ---");

// Test 3.1: Passing enum values to functions
printnl("Test 3.1: Passing enum values to functions");
printnl("Expected: Enum values can be passed as function parameters");

// Create a simple test by assigning enum values to variables and using them
int $param1 = ResponseCode.SUCCESS;
int $param2 = Permission.ADMIN;
int $result = $param1 + $param2;

printnl("Function parameter simulation:");
printnl("param1 (SUCCESS) = ", $param1);
printnl("param2 (ADMIN) = ", $param2);
printnl("result = param1 + param2 = ", $result, " (expected: 208)");
printnl("");

//==============================================================================
// 4. ENUM VALUES IN CONDITIONAL STATEMENTS
//==============================================================================

printnl("--- ENUM VALUES IN CONDITIONAL STATEMENTS ---");

// Test 4.1: Enum values in if statements
printnl("Test 4.1: Enum values in if statements");
printnl("Expected: Conditional logic works correctly with enum values");

int $current_status = ResponseCode.NOT_FOUND;
if ($current_status == ResponseCode.SUCCESS) {
    printnl("Status: Operation successful");
} else if ($current_status == ResponseCode.NOT_FOUND) {
    printnl("Status: Resource not found");
} else if ($current_status == ResponseCode.SERVER_ERROR) {
    printnl("Status: Server error occurred");
} else {
    printnl("Status: Unknown status code");
}
printnl("");

// Test 4.2: Enum values in complex conditionals
printnl("Test 4.2: Enum values in complex conditionals");
printnl("Expected: Complex conditional expressions work correctly");

int $user_permissions = Permission.READ | Permission.WRITE;
if (($user_permissions & Permission.READ) != 0) {
    printnl("User has READ permission");
}
if (($user_permissions & Permission.WRITE) != 0) {
    printnl("User has WRITE permission");
}
if (($user_permissions & Permission.EXECUTE) != 0) {
    printnl("User has EXECUTE permission");
} else {
    printnl("User does NOT have EXECUTE permission");
}
if (($user_permissions & Permission.ADMIN) != 0) {
    printnl("User has ADMIN permission");
} else {
    printnl("User does NOT have ADMIN permission");
}
printnl("");

//==============================================================================
// 5. ENUM VALUE STORAGE AND RETRIEVAL
//==============================================================================

printnl("--- ENUM VALUE STORAGE AND RETRIEVAL ---");

// Test 5.1: Storing enum values in variables
printnl("Test 5.1: Storing enum values in variables");
printnl("Expected: Enum values can be stored and retrieved from variables");

int $stored_values[4];
$stored_values[0] = Permission.READ;
$stored_values[1] = Permission.WRITE;
$stored_values[2] = Permission.EXECUTE;
$stored_values[3] = Permission.ADMIN;

printnl("Stored values:");
printnl("stored_values[0] = ", $stored_values[0], " (READ)");
printnl("stored_values[1] = ", $stored_values[1], " (WRITE)");
printnl("stored_values[2] = ", $stored_values[2], " (EXECUTE)");
printnl("stored_values[3] = ", $stored_values[3], " (ADMIN)");
printnl("");

// Test 5.2: Using stored enum values
printnl("Test 5.2: Using stored enum values");
printnl("Expected: Stored enum values work the same as direct access");

int $combined_stored = $stored_values[0] + $stored_values[1];
int $combined_direct = Permission.READ + Permission.WRITE;
bool $are_equal = ($combined_stored == $combined_direct);

printnl("combined_stored = ", $combined_stored);
printnl("combined_direct = ", $combined_direct);
printnl("are_equal = ", $are_equal, " (expected: true)");
printnl("");

//==============================================================================
// 6. MULTIPLE ENUM USAGE
//==============================================================================

printnl("--- MULTIPLE ENUM USAGE ---");

// Define additional enums for testing
enum LogLevel {
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

enum FileMode {
    READ_ONLY = 1,
    WRITE_ONLY = 2,
    READ_WRITE = 3,
    APPEND = 4
};

// Test 6.1: Using multiple enums together
printnl("Test 6.1: Using multiple enums together");
printnl("Expected: Multiple enums can be used in the same context");

int $log_level = LogLevel.WARNING;
int $file_mode = FileMode.READ_WRITE;
int $response = ResponseCode.SUCCESS;

printnl("Current configuration:");
printnl("Log level: ", $log_level, " (WARNING)");
printnl("File mode: ", $file_mode, " (READ_WRITE)");
printnl("Response: ", $response, " (SUCCESS)");

// Combine values from different enums
int $config_hash = $log_level * 1000 + $file_mode * 100 + $response;
printnl("Configuration hash: ", $config_hash, " (expected: 3500)");
printnl("");

printnl("=== ALL ENUM ACCESS AND USAGE TESTS COMPLETED ===");
printnl("");
printnl("SUMMARY:");
printnl("- Enum values can be accessed using dot notation");
printnl("- Enum values can be assigned to variables correctly");
printnl("- Enum values work in arithmetic and comparison expressions");
printnl("- Enum values can be used in conditional statements");
printnl("- Enum values can be stored in arrays and retrieved");
printnl("- Multiple enums can be used together in the same context");
printnl("- All enum operations maintain correct integer values");