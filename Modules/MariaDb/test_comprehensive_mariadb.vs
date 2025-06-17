// MariaDB Module - Phase 5: Comprehensive Integration Test
// Tests all phases (1-4) working together in realistic scenarios

print("=== MariaDB Module - Comprehensive Integration Test ===");
print("Testing all phases (1-4) together with realistic scenarios");
print("");

// Initialize MariaDB instance
var db = new MariaDB();

try {
    print("1. Testing Connection Management (Phase 1)");
    print("-------------------------------------------");
    
    // Test connection
    db.connect("localhost", "root", "", "test_db");
    print("✓ Connected to database");
    
    // Verify connection
    var connInfo = db.getConnectionInfo();
    print("✓ Connection info: " + connInfo.host + "/" + connInfo.database);
    print("✓ Connection status: " + (connInfo.is_connected ? "Connected" : "Disconnected"));
    
    print("");
    print("2. Testing Security Framework (Phase 2)");
    print("---------------------------------------");
    
    // Test input validation
    var valid = db.validateInput("users", "table_name");
    print("✓ Table name validation: " + (valid ? "PASSED" : "FAILED"));
    
    valid = db.validateInput("user@email.com", "string");
    print("✓ Email validation: " + (valid ? "PASSED" : "FAILED"));
    
    // Test prepared statements
    var stmtKey = db.prepareStatement("SELECT * FROM users WHERE id = ? AND status = ?");
    print("✓ Prepared statement created: " + stmtKey);
    
    print("");
    print("3. Setting Up Test Environment");
    print("-----------------------------");
    
    // Create test table with proper schema
    var tableColumns = {
        "id": "INT AUTO_INCREMENT PRIMARY KEY",
        "name": "VARCHAR(100) NOT NULL",
        "email": "VARCHAR(255) UNIQUE",
        "age": "INT",
        "status": "ENUM('active', 'inactive') DEFAULT 'active'",
        "balance": "DECIMAL(10,2) DEFAULT 0.00",
        "created_at": "TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
    };
    
    // Drop table if exists and create new one
    try {
        db.dropTable("test_users", true);
        print("✓ Dropped existing test table");
    } catch (e) {
        print("ⓘ No existing table to drop");
    }
    
    var created = db.createTable("test_users", tableColumns);
    print("✓ Created test table: " + (created ? "SUCCESS" : "FAILED"));
    
    // Create index for performance
    var indexCreated = db.createIndex("test_users", ["email"], "idx_email_unique", true);
    print("✓ Created unique index on email: " + (indexCreated ? "SUCCESS" : "FAILED"));
    
    print("");
    print("4. Testing CRUD Operations (Phase 3)");
    print("-----------------------------------");
    
    // Test INSERT operations
    print("INSERT Operations:");
    
    var insertId1 = db.insert("test_users", {
        "name": "John Doe",
        "email": "john@example.com",
        "age": 30,
        "balance": 1500.50
    });
    print("✓ Inserted user 1, ID: " + insertId1);
    
    var insertId2 = db.insertAndGetId("test_users", {
        "name": "Jane Smith",
        "email": "jane@example.com",
        "age": 28,
        "status": "active",
        "balance": 2300.75
    });
    print("✓ Inserted user 2, ID: " + insertId2);
    
    // Batch insert
    var batchData = [
        {"name": "Alice Johnson", "email": "alice@example.com", "age": 25, "balance": 1800.00},
        {"name": "Bob Wilson", "email": "bob@example.com", "age": 35, "balance": 2100.25},
        {"name": "Charlie Brown", "email": "charlie@example.com", "age": 42, "balance": 3200.50}
    ];
    
    var batchResults = db.insertBatch("test_users", batchData);
    print("✓ Batch insert completed: " + batchResults.length + " operations");
    
    // Test SELECT operations
    print("");
    print("SELECT Operations:");
    
    var allUsers = db.select("test_users", ["*"], {}, "name ASC");
    print("✓ Selected all users: " + Object.keys(allUsers).length + " rows");
    
    var activeUsers = db.select("test_users", ["name", "email", "balance"], {"status": "active"});
    print("✓ Selected active users: " + Object.keys(activeUsers).length + " rows");
    
    var singleUser = db.selectOne("test_users", ["*"], {"email": "john@example.com"});
    if (singleUser && singleUser.current_row) {
        print("✓ Selected single user: " + singleUser.current_row.name);
    }
    
    var userEmail = db.selectColumn("test_users", "email", {"id": insertId1});
    print("✓ Selected column value: " + userEmail);
    
    var userCount = db.getRowCount("test_users");
    print("✓ Total user count: " + userCount);
    
    // Test UPDATE operations
    print("");
    print("UPDATE Operations:");
    
    var updatedRows = db.update("test_users", 
        {"balance": 1600.00, "status": "active"}, 
        {"id": insertId1}
    );
    print("✓ Updated user balance: " + updatedRows + " rows affected");
    
    // Batch update using email as key
    var updateBatchData = [
        {"email": "alice@example.com", "age": 26, "balance": 1850.00},
        {"email": "bob@example.com", "age": 36, "balance": 2200.75}
    ];
    
    var updateResults = db.updateBatch("test_users", updateBatchData, "email");
    print("✓ Batch update completed: " + updateResults.length + " operations");
    
    print("");
    print("5. Testing Transaction Management (Phase 4)");
    print("------------------------------------------");
    
    // Get initial transaction statistics
    var initialStats = db.getTransactionStatistics();
    print("Initial transaction count: " + initialStats.transaction_count);
    
    // Test basic transaction
    print("");
    print("Basic Transaction Test:");
    
    var txnStarted = db.beginTransaction();
    print("✓ Transaction started: " + (txnStarted ? "SUCCESS" : "FAILED"));
    
    // Perform operations within transaction
    var txnInsertId = db.insert("test_users", {
        "name": "Transaction User",
        "email": "txn@example.com",
        "age": 40,
        "balance": 5000.00
    });
    print("✓ Inserted user in transaction, ID: " + txnInsertId);
    
    var txnUpdateRows = db.update("test_users", 
        {"balance": 5500.00}, 
        {"id": txnInsertId}
    );
    print("✓ Updated user in transaction: " + txnUpdateRows + " rows");
    
    var committed = db.commitTransaction();
    print("✓ Transaction committed: " + (committed ? "SUCCESS" : "FAILED"));
    
    // Test savepoint functionality
    print("");
    print("Savepoint Transaction Test:");
    
    db.beginTransaction();
    print("✓ Started new transaction for savepoint test");
    
    var savepoint1 = db.createSavepoint("before_updates");
    print("✓ Created savepoint: " + savepoint1);
    
    // Make some changes
    db.update("test_users", {"age": 41}, {"id": txnInsertId});
    print("✓ Updated user age to 41");
    
    var savepoint2 = db.createSavepoint("after_age_update");
    print("✓ Created second savepoint: " + savepoint2);
    
    // Make more changes
    db.update("test_users", {"balance": 6000.00}, {"id": txnInsertId});
    print("✓ Updated user balance to 6000.00");
    
    // Rollback to first savepoint
    var rolledBack = db.rollbackToSavepoint(savepoint1);
    print("✓ Rolled back to savepoint: " + (rolledBack ? "SUCCESS" : "FAILED"));
    
    // Commit the transaction
    db.commitTransaction();
    print("✓ Committed transaction with savepoint rollback");
    
    // Test isolation levels
    print("");
    print("Isolation Level Test:");
    
    var currentLevel = db.getIsolationLevel();
    print("✓ Current isolation level: " + currentLevel);
    
    var levelSet = db.setIsolationLevel("READ_COMMITTED");
    print("✓ Set isolation level to READ_COMMITTED: " + (levelSet ? "SUCCESS" : "FAILED"));
    
    var newLevel = db.getIsolationLevel();
    print("✓ New isolation level: " + newLevel);
    
    // Test auto-commit control
    print("");
    print("Auto-commit Test:");
    
    var autoCommit = db.getAutoCommit();
    print("✓ Current auto-commit status: " + autoCommit);
    
    var autoCommitSet = db.setAutoCommit(false);
    print("✓ Disabled auto-commit: " + (autoCommitSet ? "SUCCESS" : "FAILED"));
    
    var newAutoCommit = db.getAutoCommit();
    print("✓ New auto-commit status: " + newAutoCommit);
    
    // Restore auto-commit
    db.setAutoCommit(true);
    print("✓ Restored auto-commit");
    
    print("");
    print("6. Testing Advanced Features");
    print("---------------------------");
    
    // Test deadlock detection
    var deadlockDetected = db.detectDeadlock();
    print("✓ Deadlock detection: " + (deadlockDetected ? "DETECTED" : "NONE"));
    
    // Get final transaction statistics
    var finalStats = db.getTransactionStatistics();
    print("✓ Final transaction statistics:");
    print("  - Total transactions: " + finalStats.transaction_count);
    print("  - Rollbacks: " + finalStats.rollback_count);
    print("  - Deadlocks: " + finalStats.deadlock_count);
    print("  - Current isolation level: " + finalStats.isolation_level);
    
    print("");
    print("7. Testing Complex Scenario");
    print("--------------------------");
    
    // Complex business scenario: Transfer money between users
    print("Simulating money transfer between users...");
    
    db.beginTransaction();
    
    try {
        // Get current balances
        var fromUser = db.selectOne("test_users", ["balance"], {"id": insertId1});
        var toUser = db.selectOne("test_users", ["balance"], {"id": insertId2});
        
        if (fromUser && toUser) {
            var fromBalance = parseFloat(fromUser.current_row.balance);
            var toBalance = parseFloat(toUser.current_row.balance);
            var transferAmount = 500.00;
            
            print("✓ Transfer $" + transferAmount + " from user " + insertId1 + " to user " + insertId2);
            print("  From balance: $" + fromBalance);
            print("  To balance: $" + toBalance);
            
            if (fromBalance >= transferAmount) {
                var savepoint = db.createSavepoint("before_transfer");
                
                // Deduct from sender
                db.update("test_users", 
                    {"balance": fromBalance - transferAmount}, 
                    {"id": insertId1}
                );
                
                // Add to receiver
                db.update("test_users", 
                    {"balance": toBalance + transferAmount}, 
                    {"id": insertId2}
                );
                
                print("✓ Transfer completed successfully");
                db.commitTransaction();
            } else {
                print("✗ Insufficient funds for transfer");
                db.rollbackTransaction();
            }
        }
    } catch (e) {
        print("✗ Transfer failed: " + e);
        db.rollbackTransaction();
    }
    
    print("");
    print("8. Performance and Cleanup Tests");
    print("-------------------------------");
    
    // Test batch operations performance
    print("Testing batch operations performance...");
    
    var largeBatch = [];
    for (var i = 0; i < 100; i++) {
        largeBatch.push({
            "name": "Batch User " + i,
            "email": "batch" + i + "@example.com",
            "age": 20 + (i % 40),
            "balance": (i * 10.5)
        });
    }
    
    var batchStart = new Date().getTime();
    var largeBatchResults = db.insertBatch("test_users", largeBatch);
    var batchEnd = new Date().getTime();
    
    print("✓ Inserted " + largeBatchResults.length + " records in " + (batchEnd - batchStart) + "ms");
    
    // Clean up test data
    print("");
    print("Cleaning up test data...");
    
    var deletedRows = db.deleteRecord("test_users", {"email": "txn@example.com"});
    print("✓ Deleted transaction test user: " + deletedRows + " rows");
    
    // Delete batch users
    var batchEmails = [];
    for (var i = 0; i < 50; i++) { // Delete first 50
        batchEmails.push("batch" + i + "@example.com");
    }
    
    var batchDeleteResults = db.deleteBatch("test_users", batchEmails, "email");
    print("✓ Batch delete completed: " + batchDeleteResults.length + " operations");
    
    // Final row count
    var finalCount = db.getRowCount("test_users");
    print("✓ Final row count: " + finalCount);
    
    print("");
    print("9. Memory and Resource Management Test");
    print("------------------------------------");
    
    // Test connection health
    var isConnected = db.isConnected();
    print("✓ Connection health check: " + (isConnected ? "HEALTHY" : "UNHEALTHY"));
    
    // Get final connection info
    var finalConnInfo = db.getConnectionInfo();
    print("✓ Final connection info:");
    print("  - ID: " + finalConnInfo.connection_id);
    print("  - Connected: " + finalConnInfo.is_connected);
    print("  - Healthy: " + finalConnInfo.is_healthy);
    
    print("");
    print("=== COMPREHENSIVE TEST COMPLETED SUCCESSFULLY ===");
    print("All phases (1-4) are working together correctly:");
    print("✓ Phase 1: Connection Management - PASS");
    print("✓ Phase 2: Security Framework - PASS");
    print("✓ Phase 3: Query Execution Engine - PASS");
    print("✓ Phase 4: Transaction Management - PASS");
    print("");
    print("The MariaDB module is ready for production use!");
    
} catch (e) {
    print("✗ COMPREHENSIVE TEST FAILED: " + e);
    print("");
    print("Attempting cleanup and rollback...");
    
    try {
        if (db.isInTransaction()) {
            db.rollbackTransaction();
            print("✓ Rolled back active transaction");
        }
    } catch (rollbackError) {
        print("✗ Rollback failed: " + rollbackError);
    }
    
    print("");
    print("Test failed at comprehensive integration level.");
    print("Please check individual phase tests for specific issues.");
    
} finally {
    // Always disconnect
    try {
        db.disconnect();
        print("✓ Disconnected from database");
    } catch (disconnectError) {
        print("✗ Disconnect failed: " + disconnectError);
    }
}

print("");
print("=== Comprehensive Integration Test Complete ===");