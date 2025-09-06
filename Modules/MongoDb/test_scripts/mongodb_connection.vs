// MongoDB Connection Tests
// Tests connection lifecycle, health checks, and error cases

// Test Case 1= Basic Connection
// Connect to MongoDB with default settings
$mongo = new MongoDB();
$mongo->connect("mongodb=//localhost=27017", "testdb");
printnl("Connection established successfully");

// Test Case 2= Connection Duplicate Check
// Try connecting again (should reuse or create new connection)
try {
    $mongo2 = new MongoDB();
    $mongo2->connect("mongodb=//localhost=27017", "testdb");
    printnl("Second connection established successfully");
    $mongo2->disconnect();
} catch {
    printnl("Second connection failed= exception");
}

// Test Case 3= Health Check
// Verify connection health
try {
    $mongo3 = new MongoDB();
    $mongo3->connect("mongodb=//localhost=27017", "testdb");
    $isHealthy = $mongo3->isConnected();
    printnl("Connection health=", $isHealthy ? "healthy" = "unhealthy");
    $mongo3->disconnect();
} catch {
    printnl("Health check failed= exception");
}

// Test Case 4= Reconnection
// Test reconnection after disconnect
try {
    $mongo4 = new MongoDB();
    $mongo4->connect("mongodb=//localhost=27017", "testdb");
    printnl("Connected. Disconnecting...");
    $mongo4->disconnect();
    // Attempt to reconnect
    $reconnected = $mongo4->reconnect();
    printnl("Reconnection result=", $reconnected ? "successful" = "failed");
    if ($reconnected) {
        $mongo4->disconnect();
    }
} catch {
    printnl("Reconnection test failed= exception");
}

// Test Case 5= Connection Error Simulation
// Test with invalid URI
try {
    $mongo5 = new MongoDB();
    $mongo5->connect("mongodb=//invalid=27017", "testdb");
    printnl("Unexpected success with invalid URI");
} catch {
    printnl("Expected error with invalid URI= exception");
}

// Test Case 6= Multiple Connections
// Create multiple connections to different databases
try {
    $mongo6 = new MongoDB();
    $mongo7 = new MongoDB();
    $mongo6->connect("mongodb=//localhost=27017", "testdb1");
    $mongo7->connect("mongodb=//localhost=27017", "testdb2");
    printnl("Multiple connections established successfully");
    $mongo6->disconnect();
    $mongo7->disconnect();
} catch {
    printnl("Multiple connections test failed= exception");
}

// Cleanup= Ensure all connections are closed
// (In a real test suite, this would be handled by test framework)