// MariaDB Query Test
// Tests SELECT and non-SELECT queries using both legacy functions and OOP interface

printnl("=== MariaDB Query Test ===");

// Setup database connection
$db : new MariaDBConnection();
$db->construct();

const string $host = "localhost";
const string $username = "testuser";
const string $password = "testpass";
const string $database = "testdb";

try {
    boolean $connected = $db->connect($host, $username, $password, $database);
    if (!$connected) {
        printnl("✗ Failed to connect to database. Please check your credentials.");
        return;
    }
    printnl("✓ Connected to database successfully");
} catch (string $e) {
    printnl("✗ Connection failed:", $e);
    return;
}

// Test Case 1: CREATE TABLE (non-SELECT query)
printnl("\n--- Test Case 1: CREATE TABLE ---");
try {
    const string $createTableSQL = "
        CREATE TABLE IF NOT EXISTS test_users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            email VARCHAR(100) UNIQUE,
            age INT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ";

    int $affectedRows = $db->query($createTableSQL);
    printnl("✓ Table creation result: affected rows =", $affectedRows);
} catch (string $e) {
    printnl("✗ CREATE TABLE failed:", $e);
}

// Test Case 2: INSERT data (non-SELECT query)
printnl("\n--- Test Case 2: INSERT Data ---");
try {
    const string $insertSQL = "
        INSERT INTO test_users (name, email, age) VALUES
        ('John Doe', 'john@example.com', 25),
        ('Jane Smith', 'jane@example.com', 30),
        ('Bob Johnson', 'bob@example.com', 35)
    ";

    int $affectedRows = $db->query($insertSQL);
    printnl("✓ INSERT result: affected rows =", $affectedRows);

    // Get the last insert ID
    int $lastId = $db->getLastInsertId();
    printnl("✓ Last insert ID:", $lastId);
} catch (string $e) {
    printnl("✗ INSERT failed:", $e);
}

// Test Case 3: SELECT all data (SELECT query)
printnl("\n--- Test Case 3: SELECT All Data ---");
try {
    const string $selectSQL = "SELECT id, name, email, age, created_at FROM test_users ORDER BY id";
    object $result = $db->query($selectSQL);

    printnl("✓ SELECT result type: object with", sizeof($result), "rows");

    // Iterate through results
    for (string $rowKey : keys($result)) {
        object $row = $result[$rowKey];
        printnl("  Row", $rowKey, ": ID =", $row["id"], ", Name =", $row["name"], ", Email =", $row["email"], ", Age =", $row["age"]);
    }
} catch (string $e) {
    printnl("✗ SELECT failed:", $e);
}

// Test Case 4: SELECT with WHERE clause (SELECT query)
printnl("\n--- Test Case 4: SELECT with WHERE ---");
try {
    const string $selectWhereSQL = "SELECT name, email FROM test_users WHERE age > 25 ORDER BY age DESC";
    object $result = $db->query($selectWhereSQL);

    printnl("✓ Filtered SELECT result:", sizeof($result), "rows found");

    for (string $rowKey : keys($result)) {
        object $row = $result[$rowKey];
        printnl("  User:", $row["name"], "(", $row["email"], ")");
    }
} catch (string $e) {
    printnl("✗ Filtered SELECT failed:", $e);
}

// Test Case 5: UPDATE data (non-SELECT query)
printnl("\n--- Test Case 5: UPDATE Data ---");
try {
    const string $updateSQL = "UPDATE test_users SET age = age + 1 WHERE name LIKE 'John%'";
    int $affectedRows = $db->query($updateSQL);

    printnl("✓ UPDATE result: affected rows =", $affectedRows);

    // Verify the update
    const string $verifySQL = "SELECT name, age FROM test_users WHERE name LIKE 'John%'";
    object $verifyResult = $db->query($verifySQL);

    for (string $rowKey : keys($verifyResult)) {
        object $row = $verifyResult[$rowKey];
        printnl("  Updated user:", $row["name"], ", new age =", $row["age"]);
    }
} catch (string $e) {
    printnl("✗ UPDATE failed:", $e);
}

// Test Case 6: DELETE data (non-SELECT query)
printnl("\n--- Test Case 6: DELETE Data ---");
try {
    const string $deleteSQL = "DELETE FROM test_users WHERE age > 35";
    int $affectedRows = $db->query($deleteSQL);

    printnl("✓ DELETE result: affected rows =", $affectedRows);
} catch (string $e) {
    printnl("✗ DELETE failed:", $e);
}

// Test Case 7: Legacy function queries
printnl("\n--- Test Case 7: Legacy Function Queries ---");
try {
    // Legacy SELECT
    const string $legacySelectSQL = "SELECT COUNT(*) as total_users FROM test_users";
    object $legacyResult = mariadbQuery($legacySelectSQL);

    printnl("✓ Legacy SELECT result:");
    for (string $rowKey : keys($legacyResult)) {
        object $row = $legacyResult[$rowKey];
        printnl("  Total users:", $row["total_users"]);
    }

    // Legacy INSERT
    const string $legacyInsertSQL = "INSERT INTO test_users (name, email, age) VALUES ('Legacy User', 'legacy@example.com', 40)";
    int $legacyAffected = mariadbQuery($legacyInsertSQL);
    printnl("✓ Legacy INSERT result: affected rows =", $legacyAffected);

} catch (string $e) {
    printnl("✗ Legacy function test failed:", $e);
}

// Cleanup
printnl("\n--- Cleanup ---");
try {
    const string $dropTableSQL = "DROP TABLE IF EXISTS test_users";
    int $dropResult = $db->query($dropTableSQL);
    printnl("✓ Table cleanup: affected rows =", $dropResult);
} catch (string $e) {
    printnl("✗ Cleanup failed:", $e);
}

// Disconnect
$db->disconnect();
printnl("✓ Database connection closed");

printnl("\n=== Query Test Complete ===");