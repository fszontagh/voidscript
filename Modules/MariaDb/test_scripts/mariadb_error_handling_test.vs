// MariaDB Error Handling Test
// Tests various error scenarios and how they are handled by the module

printnl("=== MariaDB Error Handling Test ===");

// Test Case 1: Invalid connection credentials
printnl("\n--- Test Case 1: Invalid Connection Credentials ---");
try {
    $db : new MariaDBConnection();
    $db->construct();

    // Try to connect with invalid credentials
    const string $invalidHost = "invalid.host.com";
    const string $invalidUser = "nonexistent_user";
    const string $invalidPass = "wrong_password";
    const string $invalidDB = "nonexistent_database";

    printnl("Attempting connection with invalid credentials...");
    boolean $connected = $db->connect($invalidHost, $invalidUser, $invalidPass, $invalidDB);

    if ($connected) {
        printnl("✗ Unexpected success with invalid credentials");
        $db->disconnect();
    } else {
        printnl("✓ Connection correctly failed with invalid credentials");
    }
} catch (string $e) {
    printnl("✓ Expected error caught for invalid credentials:", $e);
}

// Test Case 2: Query on disconnected connection
printnl("\n--- Test Case 2: Query on Disconnected Connection ---");
try {
    $db : new MariaDBConnection();
    $db->construct();

    // Don't connect, try to query directly
    printnl("Attempting query without connection...");
    const string $testSQL = "SELECT 1";
    object $result = $db->query($testSQL);

    printnl("✗ Unexpected success querying without connection");
} catch (string $e) {
    printnl("✓ Expected error caught for query without connection:", $e);
}

// Test Case 3: SQL syntax error
printnl("\n--- Test Case 3: SQL Syntax Error ---");
try {
    // First establish a valid connection (assuming test database exists)
    $db : new MariaDBConnection();
    $db->construct();

    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    boolean $connected = $db->connect($host, $username, $password, $database);
    if (!$connected) {
        printnl("⚠ Skipping syntax error test - cannot connect to database");
        return;
    }

    // Try invalid SQL syntax
    printnl("Attempting query with invalid SQL syntax...");
    const string $invalidSQL = "SELET * FORM nonexistent_table WHRE id = 1"; // Multiple syntax errors
    object $result = $db->query($invalidSQL);

    printnl("✗ Unexpected success with invalid SQL syntax");
    $db->disconnect();
} catch (string $e) {
    printnl("✓ Expected error caught for SQL syntax error:", $e);
}

// Test Case 4: Invalid table/column names
printnl("\n--- Test Case 4: Invalid Table/Column Names ---");
try {
    $db : new MariaDBConnection();
    $db->construct();

    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    boolean $connected = $db->connect($host, $username, $password, $database);
    if (!$connected) {
        printnl("⚠ Skipping invalid table test - cannot connect to database");
        return;
    }

    // Try to select from non-existent table
    printnl("Attempting query on non-existent table...");
    const string $nonExistentSQL = "SELECT * FROM definitely_does_not_exist_table_12345";
    object $result = $db->query($nonExistentSQL);

    printnl("✗ Unexpected success querying non-existent table");
    $db->disconnect();
} catch (string $e) {
    printnl("✓ Expected error caught for non-existent table:", $e);
}

// Test Case 5: Legacy function error handling
printnl("\n--- Test Case 5: Legacy Function Error Handling ---");
try {
    // Try legacy connect with invalid parameters
    printnl("Attempting legacy connect with invalid credentials...");
    boolean $legacyConnected = mariadbConnect("invalid.host", "baduser", "badpass", "baddb");

    if ($legacyConnected) {
        printnl("✗ Unexpected legacy connection success");
        mariadbDisconnect();
    } else {
        printnl("✓ Legacy connection correctly failed");
    }
} catch (string $e) {
    printnl("✓ Expected error caught for legacy connection:", $e);
}

try {
    // Try legacy query without connection
    printnl("Attempting legacy query without connection...");
    const string $legacySQL = "SELECT 1";
    object $legacyResult = mariadbQuery($legacySQL);

    printnl("✗ Unexpected legacy query success without connection");
} catch (string $e) {
    printnl("✓ Expected error caught for legacy query without connection:", $e);
}

// Test Case 6: Connection timeout/network issues
printnl("\n--- Test Case 6: Connection Timeout/Network Issues ---");
try {
    $db : new MariaDBConnection();
    $db->construct();

    // Try to connect to a host that should cause timeout
    printnl("Attempting connection to unreachable host...");
    boolean $timeoutConnected = $db->connect("192.0.2.1", "user", "pass", "db"); // RFC 5737 test address

    if ($timeoutConnected) {
        printnl("✗ Unexpected success connecting to unreachable host");
        $db->disconnect();
    } else {
        printnl("✓ Connection correctly failed for unreachable host");
    }
} catch (string $e) {
    printnl("✓ Expected error caught for unreachable host:", $e);
}

// Test Case 7: Invalid method calls on connection object
printnl("\n--- Test Case 7: Invalid Method Calls ---");
try {
    $db : new MariaDBConnection();
    // Don't call construct() - try to use methods directly

    printnl("Attempting method call on uninitialized connection...");
    boolean $isConnected = $db->isConnected();

    printnl("✗ Unexpected success calling method on uninitialized connection");
} catch (string $e) {
    printnl("✓ Expected error caught for uninitialized connection:", $e);
}

printnl("\n=== Error Handling Test Complete ===");
printnl("Note: Some tests may be skipped if database connection cannot be established.");
printnl("This demonstrates proper error handling for various failure scenarios.");