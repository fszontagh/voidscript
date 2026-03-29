// MariaDB Basic Connection Test
// Tests basic connection functionality using both legacy functions and OOP interface

printnl("=== MariaDB Basic Connection Test ===");

// Test Case 1: Legacy function connection
printnl("\n--- Test Case 1: Legacy Function Connection ---");
try {
    // Note: Replace these with your actual database credentials
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using legacy functions...");
    boolean $connected = mariadbConnect($host, $username, $password, $database);

    if ($connected) {
        printnl("✓ Legacy connection successful");

        // Test connection status
        printnl("Connection status check would be performed here");

        // Disconnect
        mariadbDisconnect();
        printnl("✓ Legacy disconnection successful");
    } else {
        printnl("✗ Legacy connection failed");
    }
} catch (string $e) {
    printnl("✗ Legacy connection test failed with error:", $e);
}

// Test Case 2: OOP interface connection
printnl("\n--- Test Case 2: OOP Interface Connection ---");
try {
    // Create connection object
    $db : new MariaDBConnection();
    $db->construct();

    printnl("MariaDBConnection object created");

    // Attempt connection
    const string $host = "localhost";
    const string $username = "testuser";
    const string $password = "testpass";
    const string $database = "testdb";

    printnl("Attempting to connect using OOP interface...");
    boolean $connected = $db->connect($host, $username, $password, $database);

    if ($connected) {
        printnl("✓ OOP connection successful");

        // Check connection status
        boolean $isConnected = $db->isConnected();
        printnl("Connection status:", $isConnected ? "connected" : "disconnected");

        // Disconnect
        $db->disconnect();
        printnl("✓ OOP disconnection successful");

        // Verify disconnection
        boolean $stillConnected = $db->isConnected();
        printnl("After disconnect, connection status:", $stillConnected ? "still connected" : "disconnected");
    } else {
        printnl("✗ OOP connection failed");
    }
} catch (string $e) {
    printnl("✗ OOP connection test failed with error:", $e);
}

printnl("\n=== Connection Test Complete ===");
printnl("Note: This test uses placeholder credentials. Update with your actual database details.");