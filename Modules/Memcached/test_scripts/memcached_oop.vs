// Memcached OOP Interface Comprehensive Tests
// Demonstrates full OOP-style usage of MemcachedConnection

printnl("Starting Memcached OOP Interface Tests");

// Create connection object using OOP interface
$cache : new MemcachedConnection();
$cache->construct("localhost:11211");

printnl("Connection object created and connected");

// Test 1: Basic OOP operations
printnl("=== Basic OOP Operations ===");

try {
    // Set values using OOP method calls
    bool $result1 = $cache->set("user:name", "John Doe", 3600);
    bool $result2 = $cache->set("user:age", "30", 3600);
    bool $result3 = $cache->set("user:email", "john@example.com", 3600);

    printnl("Set operations results:", $result1, $result2, $result3);

    // Get values back
    string $name = $cache->get("user:name");
    string $age = $cache->get("user:age");
    string $email = $cache->get("user:email");

    printnl("Retrieved user data:");
    printnl("Name:", $name);
    printnl("Age:", $age);
    printnl("Email:", $email);

} catch (string $e) {
    printnl("Basic OOP operations error:", $e);
}

// Test 2: Advanced OOP operations with conditional logic
printnl("=== Advanced OOP Operations ===");

try {
    // Test conditional operations
    bool $added = $cache->add("test:add", "conditional_value", 300);
    printnl("Add conditional result (expected true):", $added);

    // Try to add again (should fail)
    bool $addedAgain = $cache->add("test:add", "second_value", 300);
    printnl("Add existing key result (expected false):", $addedAgain);

    // Replace operation
    bool $replaced = $cache->replace("test:add", "replaced_value", 300);
    printnl("Replace result:", $replaced);

    string $replacedValue = $cache->get("test:add");
    printnl("Replaced value:", $replacedValue);

} catch (string $e) {
    printnl("Advanced OOP operations error:", $e);
}

// Test 3: Numeric operations using OOP
printnl("=== Numeric OOP Operations ===");

try {
    // Set initial counter value
    $cache->set("counter", "100", 3600);

    // Increment operations
    int $incr1 = $cache->incr("counter");
    int $incr2 = $cache->incr("counter", 5);

    printnl("Increment results:", $incr1, $incr2);

    // Decrement operations
    int $decr1 = $cache->decr("counter");
    int $decr2 = $cache->decr("counter", 10);

    printnl("Decrement results:", $decr1, $decr2);

    // Final counter value
    string $finalCounter = $cache->get("counter");
    printnl("Final counter value:", $finalCounter);

} catch (string $e) {
    printnl("Numeric OOP operations error:", $e);
}

// Test 4: Object-oriented data structures
printnl("=== OOP Data Structures ===");

try {
    // Store object-like data
    $cache->set("user:profile:name", "Alice", 3600);
    $cache->set("user:profile:role", "admin", 3600);
    $cache->set("user:profile:last_login", "2024-09-06", 3600);

    // Retrieve and display as pseudo-object
    string $profileName = $cache->get("user:profile:name");
    string $profileRole = $cache->get("user:profile:role");
    string $lastLogin = $cache->get("user:profile:last_login");

    printnl("User Profile:");
    printnl("- Name:", $profileName);
    printnl("- Role:", $profileRole);
    printnl("- Last Login:", $lastLogin);

} catch (string $e) {
    printnl("OOP data structures error:", $e);
}

// Test 5: Connection methods
printnl("=== OOP Connection Methods ===");

try {
    // Check connection status
    bool $connected = $cache->isConnected();
    printnl("Connection status:", $connected ? "active" : "inactive");

    // Perform operations
    $cache->set("status_test", "value", 60);
    string $testValue = $cache->get("status_test");
    printnl("Status test value:", $testValue);

} catch (string $e) {
    printnl("OOP connection methods error:", $e);
}

// Test 6: Batch operations simulation
printnl("=== OOP Batch Operations Simulation ===");

try {
    // Simulate batch set by multiple individual sets
    $cache->set("batch:key1", "value1", 300);
    $cache->set("batch:key2", "value2", 300);
    $cache->set("batch:key3", "value3", 300);

    // Simulate batch get
    string $val1 = $cache->get("batch:key1");
    string $val2 = $cache->get("batch:key2");
    string $val3 = $cache->get("batch:key3");

    printnl("Batch simulation results:");
    printnl("key1:", $val1, "key2:", $val2, "key3:", $val3);

} catch (string $e) {
    printnl("OOP batch operations simulation error:", $e);
}

// Test 7: Expiration and cleanup
printnl("=== OOP Expiration Tests ===");

try {
    // Set keys with different expirations
    $cache->set("short_lived", "expires_soon", 2);
    $cache->set("long_lived", "persists", 3600);

    printnl("Set keys with different expirations");

    // In real scenario, we'd wait and check expiration
    // For demonstration, just verify they exist now
    string $shortValue = $cache->get("short_lived");
    string $longValue = $cache->get("long_lived");

    printnl("Expiration test values:");
    printnl("Short-lived:", $shortValue);
    printnl("Long-lived:", $longValue);

} catch (string $e) {
    printnl("OOP expiration tests error:", $e);
}

// Cleanup all test data using delete operations
printnl("=== Cleanup ===");

try {
    // Delete test keys
    bool $del1 = $cache->delete("user:name");
    bool $del2 = $cache->delete("user:age");
    bool $del3 = $cache->delete("user:email");
    bool $del4 = $cache->delete("test:add");
    bool $del5 = $cache->delete("counter");

    printnl("Cleanup delete results:", $del1, $del2, $del3, $del4, $del5);

    // Use flush for remaining keys (if available)
    // Note: flush method might not be implemented yet

} catch (string $e) {
    printnl("Cleanup error:", $e);
}

// Disconnect
$cache->disconnect();
printnl("OOP connection closed");