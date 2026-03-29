// MariaDB Comprehensive Test
// Complete demonstration of all MariaDB module functionality
// Covers all methods: connect, disconnect, isConnected, query, getLastInsertId, getAffectedRows

printnl("=== MariaDB Comprehensive Functionality Test ===");

// Setup test database connection
const string $host = "localhost";
const string $username = "testuser";
const string $password = "testpass";
const string $database = "testdb";

printnl("Test database configuration:");
printnl("  Host:", $host);
printnl("  Username:", $username);
printnl("  Database:", $database);

// ============================================================================
// OOP INTERFACE TESTS
// ============================================================================

printnl("\n" + "="*60);
printnl("OOP INTERFACE TESTS");
printnl("="*60);

// Test Case 1: Full OOP workflow
printnl("\n--- Test Case 1: Complete OOP Workflow ---");

$db : new MariaDBConnection();
$db->construct();

try {
    // 1. Connect
    printnl("1. Connecting to database...");
    boolean $connected = $db->connect($host, $username, $password, $database);

    if (!$connected) {
        printnl("✗ Connection failed - skipping remaining tests");
        return;
    }
    printnl("✓ Connected successfully");

    // 2. Verify connection status
    printnl("2. Checking connection status...");
    boolean $isConnected = $db->isConnected();
    printnl("   Connection status:", $isConnected ? "CONNECTED" : "DISCONNECTED");

    // 3. Create test table
    printnl("3. Creating test table...");
    const string $createTableSQL = "
        CREATE TABLE IF NOT EXISTS comprehensive_test (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            email VARCHAR(150) UNIQUE,
            age INT DEFAULT 0,
            salary DECIMAL(10,2),
            active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        ) ENGINE=InnoDB
    ";

    int $createResult = $db->query($createTableSQL);
    printnl("   Table creation affected rows:", $createResult);

    // 4. Insert test data
    printnl("4. Inserting test data...");
    const string $insertSQL = "
        INSERT INTO comprehensive_test (name, email, age, salary, active) VALUES
        ('Alice Johnson', 'alice@example.com', 28, 55000.00, true),
        ('Bob Smith', 'bob@example.com', 35, 65000.50, true),
        ('Carol Williams', 'carol@example.com', 42, 75000.75, false),
        ('David Brown', 'david@example.com', 29, 48000.25, true)
    ";

    int $insertResult = $db->query($insertSQL);
    printnl("   Insert affected rows:", $insertResult);

    // 5. Get last insert ID
    printnl("5. Getting last insert ID...");
    int $lastInsertId = $db->getLastInsertId();
    printnl("   Last insert ID:", $lastInsertId);

    // 6. Select and display all data
    printnl("6. Selecting all data...");
    const string $selectAllSQL = "SELECT * FROM comprehensive_test ORDER BY id";
    object $allData = $db->query($selectAllSQL);

    printnl("   Found", sizeof($allData), "rows:");
    for (string $rowKey : keys($allData)) {
        object $row = $allData[$rowKey];
        printnl("   ID:", $row["id"], "| Name:", $row["name"], "| Age:", $row["age"], "| Salary:", $row["salary"], "| Active:", $row["active"]);
    }

    // 7. Update data
    printnl("7. Updating data (increasing salary by 10% for active employees)...");
    const string $updateSQL = "UPDATE comprehensive_test SET salary = salary * 1.10, updated_at = NOW() WHERE active = true";
    int $updateResult = $db->query($updateSQL);
    printnl("   Update affected rows:", $updateResult);

    // 8. Get affected rows from last operation
    printnl("8. Getting affected rows from last operation...");
    int $affectedRows = $db->getAffectedRows();
    printnl("   Affected rows:", $affectedRows);

    // 9. Select with conditions
    printnl("9. Selecting employees with age > 30...");
    const string $selectConditionalSQL = "SELECT name, age, salary FROM comprehensive_test WHERE age > 30 ORDER BY salary DESC";
    object $conditionalData = $db->query($selectConditionalSQL);

    printnl("   Found", sizeof($conditionalData), "employees over 30:");
    for (string $rowKey : keys($conditionalData)) {
        object $row = $conditionalData[$rowKey];
        printnl("   ", $row["name"], "(", $row["age"], "years) - $", $row["salary"]);
    }

    // 10. Delete inactive employees
    printnl("10. Deleting inactive employees...");
    const string $deleteSQL = "DELETE FROM comprehensive_test WHERE active = false";
    int $deleteResult = $db->query($deleteSQL);
    printnl("   Delete affected rows:", $deleteResult);

    // 11. Count remaining records
    printnl("11. Counting remaining records...");
    const string $countSQL = "SELECT COUNT(*) as total FROM comprehensive_test";
    object $countResult = $db->query($countSQL);

    for (string $rowKey : keys($countResult)) {
        object $row = $countResult[$rowKey];
        printnl("   Remaining records:", $row["total"]);
    }

    // 12. Disconnect
    printnl("12. Disconnecting...");
    $db->disconnect();
    printnl("✓ Disconnected successfully");

    // 13. Verify disconnection
    printnl("13. Verifying disconnection...");
    boolean $stillConnected = $db->isConnected();
    printnl("   Connection status after disconnect:", $stillConnected ? "CONNECTED" : "DISCONNECTED");

} catch (string $e) {
    printnl("✗ OOP test failed with error:", $e);
    try {
        $db->disconnect();
    } catch (string $disconnectError) {
        printnl("   Disconnect also failed:", $disconnectError);
    }
}

// ============================================================================
// LEGACY FUNCTION TESTS
// ============================================================================

printnl("\n" + "="*60);
printnl("LEGACY FUNCTION TESTS");
printnl("="*60);

// Test Case 2: Full legacy function workflow
printnl("\n--- Test Case 2: Complete Legacy Function Workflow ---");

try {
    // 1. Connect using legacy function
    printnl("1. Connecting using legacy mariadbConnect()...");
    boolean $legacyConnected = mariadbConnect($host, $username, $password, $database);

    if (!$legacyConnected) {
        printnl("✗ Legacy connection failed - skipping remaining legacy tests");
        return;
    }
    printnl("✓ Legacy connected successfully");

    // 2. Insert data using legacy function
    printnl("2. Inserting data using legacy mariadbQuery()...");
    const string $legacyInsertSQL = "
        INSERT INTO comprehensive_test (name, email, age, salary, active) VALUES
        ('Legacy User 1', 'legacy1@example.com', 33, 60000.00, true),
        ('Legacy User 2', 'legacy2@example.com', 27, 52000.00, true)
    ";

    int $legacyInsertResult = mariadbQuery($legacyInsertSQL);
    printnl("   Legacy insert affected rows:", $legacyInsertResult);

    // 3. Select data using legacy function
    printnl("3. Selecting data using legacy mariadbQuery()...");
    const string $legacySelectSQL = "SELECT id, name, email FROM comprehensive_test WHERE name LIKE 'Legacy%' ORDER BY id DESC";
    object $legacyData = mariadbQuery($legacySelectSQL);

    printnl("   Legacy select found", sizeof($legacyData), "rows:");
    for (string $rowKey : keys($legacyData)) {
        object $row = $legacyData[$rowKey];
        printnl("   ID:", $row["id"], "|", $row["name"], "|", $row["email"]);
    }

    // 4. Update using legacy function
    printnl("4. Updating data using legacy function...");
    const string $legacyUpdateSQL = "UPDATE comprehensive_test SET age = age + 5 WHERE name LIKE 'Legacy%'";
    int $legacyUpdateResult = mariadbQuery($legacyUpdateSQL);
    printnl("   Legacy update affected rows:", $legacyUpdateResult);

    // 5. Disconnect using legacy function
    printnl("5. Disconnecting using legacy mariadbDisconnect()...");
    mariadbDisconnect();
    printnl("✓ Legacy disconnected successfully");

} catch (string $e) {
    printnl("✗ Legacy test failed with error:", $e);
    try {
        mariadbDisconnect();
    } catch (string $disconnectError) {
        printnl("   Legacy disconnect also failed:", $disconnectError);
    }
}

// ============================================================================
// MIXED USAGE AND EDGE CASES
// ============================================================================

printnl("\n" + "="*60);
printnl("MIXED USAGE AND EDGE CASES");
printnl("="*60);

// Test Case 3: Mixed OOP and Legacy usage
printnl("\n--- Test Case 3: Mixed OOP and Legacy Usage ---");

try {
    // Use OOP to connect
    $mixedDb : new MariaDBConnection();
    $mixedDb->construct();
    $mixedDb->connect($host, $username, $password, $database);

    // Use legacy functions while OOP connection is active
    printnl("Using legacy functions with active OOP connection...");
    const string $mixedSelectSQL = "SELECT COUNT(*) as total FROM comprehensive_test";
    object $mixedResult = mariadbQuery($mixedSelectSQL);

    for (string $rowKey : keys($mixedResult)) {
        object $row = $mixedResult[$rowKey];
        printnl("   Total records via legacy function:", $row["total"]);
    }

    // Use OOP methods
    const string $mixedInsertSQL = "INSERT INTO comprehensive_test (name, email, age) VALUES ('Mixed User', 'mixed@example.com', 31)";
    int $mixedInsert = $mixedDb->query($mixedInsertSQL);
    printnl("   Insert via OOP affected rows:", $mixedInsert);

    int $mixedLastId = $mixedDb->getLastInsertId();
    printnl("   Last insert ID via OOP:", $mixedLastId);

    $mixedDb->disconnect();

} catch (string $e) {
    printnl("✗ Mixed usage test failed with error:", $e);
}

// ============================================================================
// CLEANUP
// ============================================================================

printnl("\n" + "="*60);
printnl("CLEANUP");
printnl("="*60);

// Clean up test data
try {
    $cleanupDb : new MariaDBConnection();
    $cleanupDb->construct();

    if ($cleanupDb->connect($host, $username, $password, $database)) {
        printnl("Cleaning up test table...");
        const string $dropTableSQL = "DROP TABLE IF EXISTS comprehensive_test";
        int $dropResult = $cleanupDb->query($dropTableSQL);
        printnl("✓ Cleanup completed, affected rows:", $dropResult);
        $cleanupDb->disconnect();
    } else {
        printnl("⚠ Could not connect for cleanup - manual cleanup may be required");
    }
} catch (string $e) {
    printnl("⚠ Cleanup failed:", $e);
}

printnl("\n=== Comprehensive Test Complete ===");
printnl("✓ Tested all MariaDB module methods:");
printnl("  - connect() / mariadbConnect()");
printnl("  - disconnect() / mariadbDisconnect()");
printnl("  - isConnected()");
printnl("  - query() / mariadbQuery()");
printnl("  - getLastInsertId()");
printnl("  - getAffectedRows()");
printnl("✓ Tested both OOP and legacy interfaces");
printnl("✓ Tested mixed usage scenarios");
printnl("✓ Performed proper cleanup");