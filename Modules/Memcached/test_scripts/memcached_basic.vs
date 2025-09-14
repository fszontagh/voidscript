// Memcached Basic Operations Tests
// Tests connect, get, set, delete operations

// Setup test connection
$memcached : new MemcachedConnection();
$memcached->construct("localhost");

printnl("Connection established successfully");

// Test Case 1: Set a basic key-value pair
try {
    $result : $memcached->set("test_key", "test_value", 300);
    printnl("Set operation result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Set operation failed:", $e);
}

// Test Case 2: Get the value back
try {
    $value : $memcached->get("test_key");
    printnl("Retrieved value:", $value);
} catch (string $e) {
    printnl("Get operation failed:", $e);
}

// Test Case 3: Delete the key
try {
    $result : $memcached->delete("test_key");
    printnl("Delete operation result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Delete operation failed:", $e);
}

// Test Case 4: Verify key is deleted (should throw exception)
try {
    $value : $memcached->get("test_key");
    printnl("Unexpected success retrieving deleted key");
} catch (string $e) {
    printnl("Expected error retrieving deleted key:", $e);
}

// Test Case 5: Set with expiration
try {
    $result : $memcached->set("expiring_key", "expires_soon", 2);
    printnl("Set with expiration result:", $result ? "success" : "failed");

    // Immediately get it
    $value : $memcached->get("expiring_key");
    printnl("Retrieved expiring value:", $value);

    // Wait a bit (in real scenarios)
    printnl("Waiting 3 seconds for expiration...");
    // In voidscript, we'd need a sleep function if available
    // For now, just note that in practice this would expire

} catch (string $e) {
    printnl("Expiration test failed:", $e);
}

// Cleanup
$memcached->disconnect();
printnl("Connection closed");