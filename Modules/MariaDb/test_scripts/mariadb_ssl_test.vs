// MariaDB SSL Connection Test
// Tests SSL enable/disable functionality using both legacy functions and OOP interface

printnl("=== MariaDB SSL Connection Test ===");

// Test Case 1: Legacy function connection with SSL enabled
printnl("\n--- Test Case 1: Legacy Function Connection with SSL Enabled ---");
try {
    // Note: Replace these with your actual database credentials
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using legacy functions with SSL enabled...");
    boolean $connected = mariadbConnect($host, $username, $password, $database, true);

    if ($connected) {
        printnl("✓ Legacy SSL connection successful");

        // Disconnect
        mariadbDisconnect();
        printnl("✓ Legacy SSL disconnection successful");
    } else {
        printnl("✗ Legacy SSL connection failed");
    }
} catch (string $e) {
    printnl("✗ Legacy SSL connection test failed with error:", $e);
}

// Test Case 2: Legacy function connection with SSL disabled (default)
printnl("\n--- Test Case 2: Legacy Function Connection with SSL Disabled ---");
try {
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using legacy functions with SSL disabled...");
    boolean $connected = mariadbConnect($host, $username, $password, $database, false);

    if ($connected) {
        printnl("✓ Legacy non-SSL connection successful");

        // Disconnect
        mariadbDisconnect();
        printnl("✓ Legacy non-SSL disconnection successful");
    } else {
        printnl("✗ Legacy non-SSL connection failed");
    }
} catch (string $e) {
    printnl("✗ Legacy non-SSL connection test failed with error:", $e);
}

// Test Case 3: OOP interface connection with SSL enabled
printnl("\n--- Test Case 3: OOP Interface Connection with SSL Enabled ---");
try {
    // Create connection object
    $db : new MariaDBConnection();
    $db->construct();

    printnl("MariaDBConnection object created");

    // Attempt connection with SSL
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using OOP interface with SSL enabled...");
    boolean $connected = $db->connect($host, $username, $password, $database, true);

    if ($connected) {
        printnl("✓ OOP SSL connection successful");

        // Check connection status
        boolean $isConnected = $db->isConnected();
        printnl("Connection status:", $isConnected ? "connected" : "disconnected");

        // Disconnect
        $db->disconnect();
        printnl("✓ OOP SSL disconnection successful");
    } else {
        printnl("✗ OOP SSL connection failed");
    }
} catch (string $e) {
    printnl("✗ OOP SSL connection test failed with error:", $e);
}

// Test Case 4: OOP interface connection with SSL disabled
printnl("\n--- Test Case 4: OOP Interface Connection with SSL Disabled ---");
try {
    // Create connection object
    $db2 : new MariaDBConnection();
    $db2->construct();

    printnl("MariaDBConnection object created");

    // Attempt connection without SSL
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using OOP interface with SSL disabled...");
    boolean $connected = $db2->connect($host, $username, $password, $database, false);

    if ($connected) {
        printnl("✓ OOP non-SSL connection successful");

        // Disconnect
        $db2->disconnect();
        printnl("✓ OOP non-SSL disconnection successful");
    } else {
        printnl("✗ OOP non-SSL connection failed");
    }
} catch (string $e) {
    printnl("✗ OOP non-SSL connection test failed with error:", $e);
}

// Test Case 5: Error handling - invalid SSL configuration
printnl("\n--- Test Case 5: Error Handling for SSL Configuration ---");
try {
    // Try connecting with SSL to a server that doesn't support it
    const string $host = "invalid-ssl-server.com";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect with SSL to invalid server (should fail)...");
    boolean $connected = mariadbConnect($host, $username, $password, $database, true);

    if ($connected) {
        printnl("✓ Unexpectedly connected to invalid SSL server");
        mariadbDisconnect();
    } else {
        printnl("✓ Correctly failed to connect to invalid SSL server");
    }
} catch (string $e) {
    printnl("✓ Correctly caught error for invalid SSL configuration:", $e);
}

printnl("\n=== SSL Connection Test Complete ===");
printnl("Note: This test uses placeholder credentials. Update with your actual database details.");
printnl("For SSL tests, ensure your MariaDB server supports SSL connections.");