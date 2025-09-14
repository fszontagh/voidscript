// Memcached Advanced Features Tests
// Tests CAS operations, add/replace, increment/decrement

// Setup test connection
$memcached : new MemcachedConnection();
$memcached->construct("localhost");

printnl("Connection established successfully");

// Test Case 1: Add operation (succeed if key doesn't exist)
try {
    $result : $memcached->add("add_test_key", "add_value", 300);
    printnl("Add new key result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Add operation failed:", $e);
}

// Test Case 2: Add operation again (should fail)
try {
    $result : $memcached->add("add_test_key", "new_value", 300);
    printnl("Add existing key result:", $result ? "unexpected success" : "expected failure");
} catch (string $e) {
    printnl("Expected error for add existing key:", $e);
}

// Test Case 3: Replace operation (succeed if key exists)
try {
    $result : $memcached->replace("add_test_key", "replaced_value", 300);
    printnl("Replace existing key result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Replace operation failed:", $e);
}

// Test Case 4: Replace operation on non-existent key (should fail)
try {
    $result : $memcached->replace("non_existent_key", "value", 300);
    printnl("Replace non-existent key result:", $result ? "unexpected success" : "expected failure");
} catch (string $e) {
    printnl("Expected error for replace non-existent key:", $e);
}

// Test Case 5: Increment operation
try {
    // First set a numeric value
    $memcached->set("counter", "100", 300);

    // Increment by 1
    $newValue : $memcached->incr("counter");
    printnl("Increment result (expected 101):", $newValue);

    // Increment by 5
    $newValue : $memcached->incr("counter", 5);
    printnl("Increment by 5 result (expected 106):", $newValue);

} catch (string $e) {
    printnl("Increment operation failed:", $e);
}

// Test Case 6: Decrement operation
try {
    // Decrement by 1
    $newValue : $memcached->decr("counter");
    printnl("Decrement result (expected 105):", $newValue);

    // Decrement by 10
    $newValue : $memcached->decr("counter", 10);
    printnl("Decrement by 10 result (expected 95):", $newValue);

} catch (string $e) {
    printnl("Decrement operation failed:", $e);
}

// Test Case 7: CAS (Check And Set) simulation
// Note: In voidscript, getting the CAS value would require additional API method
// For now, we'll simulate the basic pattern

try {
    // Set an initial value
    $memcached->set("cas_test", "initial", 300);

    // Get the current value
    $currentValue : $memcached->get("cas_test");
    printnl("Current value for CAS test:", $currentValue);

    // In a real CAS scenario, we'd need:
    // - A method to get the CAS token
    // - Update only if CAS token matches
    // For testing, we'll just show the pattern

    printnl("CAS operation would require CAS token retrieval API");

} catch (string $e) {
    printnl("CAS test operation failed:", $e);
}

// Test Case 8: Batch operations (if implemented)
try {
    // These would require getMulti and setMulti to be implemented
    // For now, just set multiple keys individually
    $memcached->set("batch_key1", "batch_value1", 300);
    $memcached->set("batch_key2", "batch_value2", 300);

    $value1 : $memcached->get("batch_key1");
    $value2 : $memcached->get("batch_key2");
    printnl("Batch test values:", $value1, $value2);

} catch (string $e) {
    printnl("Batch operations failed:", $e);
}

// Cleanup test data
$memcached->delete("add_test_key");
$memcached->delete("counter");
$memcached->delete("cas_test");
$memcached->delete("batch_key1");
$memcached->delete("batch_key2");

// Cleanup
$memcached->disconnect();
printnl("Connection closed");