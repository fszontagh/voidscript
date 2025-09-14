// Memcached Error Handling and Edge Cases Tests
// Tests various error conditions and edge cases

printnl("Starting Memcached Error Handling Tests");

// Test Case 1: Invalid connection (no memcached server running)
try {
    $memcached : new MemcachedConnection();
    $memcached->construct("invalid_server:11211");
    printnl("Unexpected success connecting to invalid server");
} catch (string $e) {
    printnl("Expected error connecting to invalid server:", $e);
}

// Test Case 2: Connection with empty servers string
try {
    $memcached : new MemcachedConnection();
    $memcached->construct("");
    printnl("Unexpected success connecting with empty servers");
} catch (string $e) {
    printnl("Expected error connecting with empty servers:", $e);
}

// Test Case 3: Operations on disconnected client
try {
    $memcached : new MemcachedConnection();
    // Don't connect, just try operations

    $result : $memcached->set("test_key", "value");
    printnl("Unexpected success with disconnected client:", $result);
} catch (string $e) {
    printnl("Expected error with disconnected client:", $e);
}

// Test Case 4: Valid connection for further error tests
$memcached : null;
try {
    $memcached : new MemcachedConnection();
    $memcached->construct("localhost");
    printnl("Connection established for error tests");
} catch (string $e) {
    printnl("Could not establish connection for tests:", $e);
    printnl("Skipping remaining tests due to connection failure");
    return;
}

// Test Case 5: Get non-existent key
try {
    $value : $memcached->get("non_existent_key");
    printnl("Unexpected success getting non-existent key:", $value);
} catch (string $e) {
    printnl("Expected error getting non-existent key:", $e);
}

// Test Case 6: Delete non-existent key
try {
    $result : $memcached->delete("non_existent_key");
    printnl("Delete non-existent key result:", $result ? "success" : "failed");
    // This should actually succeed (deleting non-existent key is ok)
} catch (string $e) {
    printnl("Delete non-existent key error:", $e);
}

// Test Case 7: Add existing key (should fail)
try {
    $memcached->set("existing_key", "value", 300);
    $result : $memcached->add("existing_key", "new_value", 300);
    printnl("Add existing key result:", $result ? "unexpected success" : "expected failure");
} catch (string $e) {
    printnl("Expected error adding existing key:", $e);
}

// Test Case 8: Replace non-existent key (should fail)
try {
    $result : $memcached->replace("non_existent_key", "value", 300);
    printnl("Replace non-existent key result:", $result ? "unexpected success" : "expected failure");
} catch (string $e) {
    printnl("Expected error replacing non-existent key:", $e);
}

// Test Case 9: Increment non-existent key
try {
    $result : $memcached->incr("non_existent_counter");
    printnl("Increment non-existent key result:", $result);
    // This might work if memcached auto-creates
} catch (string $e) {
    printnl("Increment non-existent key error:", $e);
}

// Test Case 10: Increment non-numeric value
try {
    $memcached->set("text_key", "not_a_number", 300);
    $result : $memcached->incr("text_key");
    printnl("Increment non-numeric result:", $result);
} catch (string $e) {
    printnl("Expected error incrementing non-numeric:", $e);
}

// Test Case 11: Decrement non-existent key
try {
    $result : $memcached->decr("non_existent_counter");
    printnl("Decrement non-existent key result:", $result);
} catch (string $e) {
    printnl("Decrement non-existent key error:", $e);
}

// Test Case 12: Decrement non-numeric value
try {
    $result : $memcached->decr("text_key");
    printnl("Decrement non-numeric result:", $result);
} catch (string $e) {
    printnl("Expected error decrementing non-numeric:", $e);
}

// Test Case 13: Set with negative expiration (might fail or interpret as absolute time)
try {
    $result : $memcached->set("negative_expiry", "value", -1);
    printnl("Set with negative expiration result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Set with negative expiration error:", $e);
}

// Test Case 14: Very long key name (memcached has length limits)
try {
    string $longKey = "";
    for (int $i = 0; $i < 300; $i += 1) {
        $longKey += "a";
    }

    $result : $memcached->set($longKey, "value", 300);
    printnl("Set with very long key result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Set with very long key error:", $e);
}

// Test Case 15: Very large value
try {
    string $largeValue = "";
    for (int $i = 0; $i < 1000000; $i += 1) {  // 1MB string
        $largeValue += "x";
    }

    $result : $memcached->set("large_value_key", $largeValue, 300);
    printnl("Set with large value result:", $result ? "success" : "failed");
} catch (string $e) {
    printnl("Set with large value error:", $e);
}

// Test Case 16: Multiple connections test
try {
    $memcached2 : new MemcachedConnection();
    $memcached2->construct("localhost");

    // Try operations on both connections
    $memcached->set("shared_key", "value1", 300);
    $memcached2->set("shared_key", "value2", 300);

    $retrieved : $memcached->get("shared_key");
    printnl("Value after multiple connections:", $retrieved);

    $memcached2->disconnect();
} catch (string $e) {
    printnl("Multiple connections test error:", $e);
}

// Test Case 17: Connection state checking
try {
    bool $isConnected = $memcached->isConnected();
    printnl("Connection status:", $isConnected ? "connected" : "disconnected");
} catch (string $e) {
    printnl("Connection status check error:", $e);
}

// Cleanup
$memcached->flush();  // Clear all test data
$memcached->disconnect();

printnl("Error handling tests completed");