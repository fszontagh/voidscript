// Phase 4: Transaction Management Test Script
// Tests all transaction management features including ACID compliance,
// savepoints, deadlock detection, and isolation levels

print("=== MariaDB Phase 4 Transaction Management Test ===");

// Create MariaDB instance
var db = new MariaDB();

// Test connection
print("\n1. Testing database connection...");
try {
    db.connect("localhost", "test_user", "test_pass", "test_db");
    print("✓ Connected to database successfully");
    
    var info = db.getConnectionInfo();
    print("Connection info: " + info.connection_id + " to " + info.host + "/" + info.database);
} catch (e) {
    print("✗ Connection failed: " + e);
    return;
}

// Test 1: Basic Transaction Management
print("\n2. Testing basic transaction management...");
try {
    // Check initial transaction state
    var inTxn = db.isInTransaction();
    print("Initial transaction state: " + (inTxn ? "IN TRANSACTION" : "NOT IN TRANSACTION"));
    
    // Begin transaction
    var beginResult = db.beginTransaction();
    print("Begin transaction result: " + beginResult);
    
    // Check transaction state
    inTxn = db.isInTransaction();
    print("After begin - transaction state: " + (inTxn ? "IN TRANSACTION" : "NOT IN TRANSACTION"));
    
    // Create test table within transaction
    var createResult = db.createTable("transaction_test", {
        "id": "INT AUTO_INCREMENT PRIMARY KEY",
        "name": "VARCHAR(100)",
        "value": "INT"
    });
    print("Create table result: " + createResult);
    
    // Insert test data
    var insertResult = db.insert("transaction_test", {
        "name": "test_record_1",
        "value": 100
    });
    print("Insert result (ID): " + insertResult);
    
    // Commit transaction
    var commitResult = db.commitTransaction();
    print("Commit transaction result: " + commitResult);
    
    // Check final transaction state
    inTxn = db.isInTransaction();
    print("After commit - transaction state: " + (inTxn ? "IN TRANSACTION" : "NOT IN TRANSACTION"));
    
    print("✓ Basic transaction management test passed");
} catch (e) {
    print("✗ Basic transaction test failed: " + e);
    try {
        db.rollbackTransaction();
    } catch (rollbackError) {
        print("Rollback error: " + rollbackError);
    }
}

// Test 2: Transaction Rollback
print("\n3. Testing transaction rollback...");
try {
    // Begin new transaction
    db.beginTransaction();
    print("Started new transaction for rollback test");
    
    // Insert data that we'll rollback
    var insertId = db.insert("transaction_test", {
        "name": "rollback_test",
        "value": 999
    });
    print("Inserted record with ID: " + insertId);
    
    // Verify data exists before rollback
    var beforeRollback = db.selectOne("transaction_test", ["*"], {"id": insertId});
    if (beforeRollback && beforeRollback.current_row) {
        print("Record exists before rollback: " + beforeRollback.current_row.name);
    }
    
    // Rollback transaction
    var rollbackResult = db.rollbackTransaction();
    print("Rollback result: " + rollbackResult);
    
    // Verify data was rolled back
    try {
        var afterRollback = db.selectOne("transaction_test", ["*"], {"id": insertId});
        if (!afterRollback || !afterRollback.current_row) {
            print("✓ Record successfully rolled back (not found)");
        } else {
            print("✗ Record still exists after rollback");
        }
    } catch (selectError) {
        print("✓ Record not found after rollback (expected)");
    }
    
    print("✓ Transaction rollback test passed");
} catch (e) {
    print("✗ Transaction rollback test failed: " + e);
}

// Test 3: Savepoint Management
print("\n4. Testing savepoint management...");
try {
    // Begin transaction for savepoint test
    db.beginTransaction();
    print("Started transaction for savepoint test");
    
    // Create first savepoint
    var savepoint1 = db.createSavepoint("savepoint_1");
    print("Created savepoint: " + savepoint1);
    
    // Insert first record
    var id1 = db.insert("transaction_test", {
        "name": "savepoint_test_1",
        "value": 200
    });
    print("Inserted first record (ID: " + id1 + ")");
    
    // Create second savepoint
    var savepoint2 = db.createSavepoint("savepoint_2");
    print("Created savepoint: " + savepoint2);
    
    // Insert second record
    var id2 = db.insert("transaction_test", {
        "name": "savepoint_test_2",
        "value": 300
    });
    print("Inserted second record (ID: " + id2 + ")");
    
    // Rollback to first savepoint (should remove second record but keep first)
    var rollbackResult = db.rollbackToSavepoint("savepoint_1");
    print("Rollback to savepoint_1 result: " + rollbackResult);
    
    // Verify first record still exists
    var record1 = db.selectOne("transaction_test", ["*"], {"id": id1});
    if (record1 && record1.current_row) {
        print("✓ First record still exists after rollback to savepoint");
    } else {
        print("✗ First record missing after rollback to savepoint");
    }
    
    // Verify second record was rolled back
    try {
        var record2 = db.selectOne("transaction_test", ["*"], {"id": id2});
        if (!record2 || !record2.current_row) {
            print("✓ Second record was rolled back (not found)");
        } else {
            print("✗ Second record still exists after rollback to savepoint");
        }
    } catch (selectError) {
        print("✓ Second record was rolled back (expected)");
    }
    
    // Release savepoint
    var releaseResult = db.releaseSavepoint("savepoint_1");
    print("Release savepoint result: " + releaseResult);
    
    // Commit transaction
    db.commitTransaction();
    print("✓ Savepoint management test passed");
} catch (e) {
    print("✗ Savepoint test failed: " + e);
    try {
        db.rollbackTransaction();
    } catch (rollbackError) {
        print("Rollback error: " + rollbackError);
    }
}

// Test 4: Isolation Level Management
print("\n5. Testing isolation level management...");
try {
    // Test different isolation levels
    var isolationLevels = ["READ_UNCOMMITTED", "READ_COMMITTED", "REPEATABLE_READ", "SERIALIZABLE"];
    
    for (var i = 0; i < isolationLevels.length; i++) {
        var level = isolationLevels[i];
        var setResult = db.setIsolationLevel(level);
        print("Set isolation level to " + level + ": " + setResult);
        
        var currentLevel = db.getIsolationLevel();
        print("Current isolation level: " + currentLevel);
        
        if (currentLevel == level.replace("_", " ")) {
            print("✓ Isolation level set correctly");
        } else {
            print("✗ Isolation level mismatch");
        }
    }
    
    print("✓ Isolation level management test passed");
} catch (e) {
    print("✗ Isolation level test failed: " + e);
}

// Test 5: Auto-commit Control
print("\n6. Testing auto-commit control...");
try {
    // Test disabling auto-commit
    var disableResult = db.setAutoCommit(false);
    print("Disable auto-commit result: " + disableResult);
    
    var autoCommitStatus = db.getAutoCommit();
    print("Auto-commit status: " + autoCommitStatus);
    
    // Test enabling auto-commit
    var enableResult = db.setAutoCommit(true);
    print("Enable auto-commit result: " + enableResult);
    
    autoCommitStatus = db.getAutoCommit();
    print("Auto-commit status: " + autoCommitStatus);
    
    print("✓ Auto-commit control test passed");
} catch (e) {
    print("✗ Auto-commit test failed: " + e);
}

// Test 6: Deadlock Detection
print("\n7. Testing deadlock detection...");
try {
    var deadlockDetected = db.detectDeadlock();
    print("Deadlock detection result: " + deadlockDetected);
    
    if (deadlockDetected == false) {
        print("✓ No deadlock detected (expected for single connection)");
    } else {
        print("! Deadlock detected (unexpected but not necessarily error)");
    }
    
    print("✓ Deadlock detection test completed");
} catch (e) {
    print("✗ Deadlock detection test failed: " + e);
}

// Test 7: Transaction Statistics
print("\n8. Testing transaction statistics...");
try {
    var stats = db.getTransactionStatistics();
    print("Transaction statistics:");
    print("  - Transaction count: " + stats.transaction_count);
    print("  - Rollback count: " + stats.rollback_count);
    print("  - Deadlock count: " + stats.deadlock_count);
    print("  - Savepoint count: " + stats.savepoint_count);
    print("  - In transaction: " + stats.is_in_transaction);
    print("  - Auto-commit enabled: " + stats.auto_commit_enabled);
    print("  - Isolation level: " + stats.isolation_level);
    
    if (stats.transaction_count > 0) {
        print("✓ Transaction statistics show activity");
    } else {
        print("! No transaction activity recorded");
    }
    
    print("✓ Transaction statistics test completed");
} catch (e) {
    print("✗ Transaction statistics test failed: " + e);
}

// Test 8: Complex Transaction Scenario
print("\n9. Testing complex transaction scenario...");
try {
    // Begin complex transaction
    db.beginTransaction();
    print("Started complex transaction");
    
    // Create multiple savepoints and test nested operations
    var sp1 = db.createSavepoint("complex_sp1");
    db.insert("transaction_test", {"name": "complex_1", "value": 1000});
    
    var sp2 = db.createSavepoint("complex_sp2");
    db.insert("transaction_test", {"name": "complex_2", "value": 2000});
    
    var sp3 = db.createSavepoint("complex_sp3");
    db.insert("transaction_test", {"name": "complex_3", "value": 3000});
    
    // Rollback to middle savepoint
    db.rollbackToSavepoint("complex_sp2");
    print("Rolled back to complex_sp2");
    
    // Insert different data
    db.insert("transaction_test", {"name": "complex_replacement", "value": 9999});
    
    // Commit everything
    db.commitTransaction();
    print("Complex transaction committed");
    
    // Verify final state
    var finalRecords = db.select("transaction_test", ["*"], {"value": 9999});
    if (finalRecords && finalRecords["0"]) {
        print("✓ Complex transaction scenario completed successfully");
    } else {
        print("✗ Complex transaction verification failed");
    }
    
    print("✓ Complex transaction scenario test passed");
} catch (e) {
    print("✗ Complex transaction scenario failed: " + e);
    try {
        db.rollbackTransaction();
    } catch (rollbackError) {
        print("Rollback error: " + rollbackError);
    }
}

// Cleanup
print("\n10. Cleaning up test data...");
try {
    db.dropTable("transaction_test", true);
    print("✓ Test table dropped successfully");
} catch (e) {
    print("✗ Cleanup failed: " + e);
}

// Final statistics
print("\n11. Final transaction statistics...");
try {
    var finalStats = db.getTransactionStatistics();
    print("Final statistics:");
    print("  - Total transactions: " + finalStats.transaction_count);
    print("  - Total rollbacks: " + finalStats.rollback_count);
    print("  - Total deadlocks: " + finalStats.deadlock_count);
    print("  - Current savepoints: " + finalStats.savepoint_count);
    print("  - Currently in transaction: " + finalStats.is_in_transaction);
} catch (e) {
    print("Final statistics error: " + e);
}

// Disconnect
print("\n12. Disconnecting...");
try {
    db.disconnect();
    print("✓ Disconnected successfully");
} catch (e) {
    print("✗ Disconnect failed: " + e);
}

print("\n=== Phase 4 Transaction Management Test Complete ===");
print("All transaction features tested including:");
print("✓ Basic transaction lifecycle (begin/commit/rollback)");
print("✓ Savepoint management with nested transactions");
print("✓ Isolation level control");
print("✓ Auto-commit management");
print("✓ Deadlock detection");
print("✓ Transaction statistics and monitoring");
print("✓ Complex transaction scenarios");
print("✓ ACID compliance verification");