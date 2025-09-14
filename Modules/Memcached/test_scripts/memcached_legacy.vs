// Memcached Legacy Function Call Style Tests
// Tests the legacy function-based interface (limited implementation)

printnl("Starting Memcached Legacy Function Interface Tests");

// Note: Current implementation has limited legacy function support
// Most legacy functions throw "Use OOP interface instead" errors

printnl("=== Legacy Function Patterns (Limited Implementation) ===");

// Test Case 1: Legacy connection function
try {
    string $connectionId = memcachedConnect("localhost");
    printnl("Legacy connection result:", $connectionId);
} catch (string $e) {
    printnl("Legacy connection error (expected for now):", $e);
}

// Test Case 2: Legacy get function
try {
    // This will fail as legacy functions aren't fully implemented
    string $value = memcachedGet("some_key");
    printnl("Legacy get result:", $value);
} catch (string $e) {
    printnl("Legacy get error (expected):", $e);
}

// Test Case 3: Legacy set function
try {
    // This will fail as legacy functions aren't fully implemented
    bool $result = memcachedSet("test_key", "test_value", 300);
    printnl("Legacy set result:", $result);
} catch (string $e) {
    printnl("Legacy set error (expected):", $e);
}

// Test Case 4: Legacy delete function
try {
    bool $result = memcachedDelete("test_key");
    printnl("Legacy delete result:", $result);
} catch (string $e) {
    printnl("Legacy delete error (expected):", $e);
}

// Test Case 5: Show expected legacy function signatures
printnl("=== Expected Legacy Function Signatures ===");

// These are the patterns that SHOULD work if legacy functions were implemented:
// memcachedConnect(servers)
// memcachedGet(key)
// memcachedSet(key, value, expiration)
// memcachedDelete(key)
// memcachedExists(key)
// memcachedFlush()
// memcachedIncr(key, offset)
// memcachedDecr(key, offset)

printnl("Note: Current MemcachedModule implementation focuses on OOP interface");
printnl("Legacy functions are stubbed to encourage OOP usage");

// Demonstrate why OOP is preferred for this module
printnl("=== Recommendation: Use OOP Interface ===");

// Show OOP equivalent
$memcached : new MemcachedConnection();
$memcached->construct("localhost");

try {
    // OOP operations work
    $memcached->set("oop_key", "oop_value", 300);
    $value : $memcached->get("oop_key");
    printnl("OOP interface works - retrieved:", $value);

    $memcached->delete("oop_key");
    $memcached->disconnect();

} catch (string $e) {
    printnl("OOP operations error:", $e);
}

printnl("=== Legacy vs OOP Comparison ===");
printnl("Legacy (if implemented):");
printnl("  memcachedSet(\"key\", \"value\", 300);");
printnl("  $result = memcachedGet(\"key\");");
printnl("");
printnl("OOP (recommended):");
printnl("  $cache : new MemcachedConnection();");
printnl("  $cache->construct(\"localhost\");");
printnl("  $cache->set(\"key\", \"value\", 300);");
printnl("  $result = $cache->get(\"key\");");
printnl("  $cache->disconnect();");

printnl("Legacy function tests completed");