#!/usr/bin/env voidscript

// MariaDB Module Phase 3 Query Execution Engine Test
// Tests comprehensive CRUD operations, batch processing, and schema operations

print("=== MariaDB Module Phase 3 Query Execution Engine Test ===");

try {
    // Initialize MariaDB module
    var db = new MariaDB();
    print("✓ MariaDB module initialized successfully");
    
    // Test connection
    print("\n--- Testing Connection ---");
    db.connect("localhost", "testuser", "testpass", "testdb");
    print("✓ Database connection established");
    
    // Verify connection
    var connInfo = db.getConnectionInfo();
    print("Connection info: " + connInfo["connection_id"]);
    print("Connected: " + connInfo["is_connected"]);
    print("Healthy: " + connInfo["is_healthy"]);
    
    // Test Phase 3: Schema Operations
    print("\n--- Testing Schema Operations ---");
    
    // Create test table
    var columns = {
        "id": "INT AUTO_INCREMENT PRIMARY KEY",
        "name": "VARCHAR(100) NOT NULL",
        "email": "VARCHAR(255) UNIQUE",
        "age": "INT",
        "created_at": "TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
    };
    
    var constraints = [
        "INDEX idx_name (name)",
        "INDEX idx_age (age)"
    ];
    
    try {
        db.dropTable("test_users", true);
        print("✓ Dropped existing test table (if any)");
    } catch (e) {
        print("Note: Table didn't exist, continuing...");
    }
    
    var createResult = db.createTable("test_users", columns, constraints);
    print("✓ Created test table: " + createResult);
    
    // Test Phase 3: INSERT Operations
    print("\n--- Testing INSERT Operations ---");
    
    // Single insert
    var userData = {
        "name": "John Doe",
        "email": "john@example.com",
        "age": 30
    };
    
    var insertId = db.insert("test_users", userData);
    print("✓ Single insert completed, ID: " + insertId);
    
    // Insert and get ID
    var userData2 = {
        "name": "Jane Smith",
        "email": "jane@example.com",
        "age": 25
    };
    
    var insertId2 = db.insertAndGetId("test_users", userData2);
    print("✓ Insert with ID completed, ID: " + insertId2);
    
    // Batch insert
    var batchData = [
        {
            "name": "Bob Johnson",
            "email": "bob@example.com",
            "age": 35
        },
        {
            "name": "Alice Brown",
            "email": "alice@example.com",
            "age": 28
        },
        {
            "name": "Charlie Wilson",
            "email": "charlie@example.com",
            "age": 42
        }
    ];
    
    var batchResult = db.insertBatch("test_users", batchData);
    print("✓ Batch insert completed, results: " + batchResult);
    
    // Test Phase 3: SELECT Operations
    print("\n--- Testing SELECT Operations ---");
    
    // Select all
    var allUsers = db.select("test_users");
    print("✓ Select all users:");
    for (var i = 0; i < 10; i++) {
        if (allUsers[i]) {
            var user = allUsers[i];
            print("  User " + i + ": " + user["name"] + " (" + user["email"] + "), Age: " + user["age"]);
        }
    }
    
    // Select with conditions
    var conditions = {
        "age": 30
    };
    
    var filteredUsers = db.select("test_users", ["name", "email", "age"], conditions);
    print("✓ Select with conditions (age = 30):");
    for (var i = 0; i < 5; i++) {
        if (filteredUsers[i]) {
            var user = filteredUsers[i];
            print("  " + user["name"] + " - " + user["email"]);
        }
    }
    
    // Select one
    var oneUser = db.selectOne("test_users", ["name", "email"], {"name": "Jane Smith"});
    if (oneUser && oneUser["current_row"]) {
        var userData = oneUser["current_row"];
        print("✓ Select one user: " + userData["name"] + " - " + userData["email"]);
    }
    
    // Select column
    var userName = db.selectColumn("test_users", "name", {"id": insertId});
    print("✓ Select column result: " + userName);
    
    // Select scalar
    var userEmail = db.selectScalar("test_users", "email", {"id": insertId2});
    print("✓ Select scalar result: " + userEmail);
    
    // Test Phase 3: UPDATE Operations
    print("\n--- Testing UPDATE Operations ---");
    
    // Single update
    var updateData = {
        "age": 31
    };
    
    var updateConditions = {
        "id": insertId
    };
    
    var affectedRows = db.update("test_users", updateData, updateConditions);
    print("✓ Single update completed, affected rows: " + affectedRows);
    
    // Batch update
    var updateBatchData = [
        {
            "id": insertId2,
            "age": 26
        },
        {
            "id": insertId + 2,
            "age": 36
        }
    ];
    
    var batchUpdateResult = db.updateBatch("test_users", updateBatchData, "id");
    print("✓ Batch update completed, results: " + batchUpdateResult);
    
    // Test Phase 3: Row Count
    print("\n--- Testing Utility Operations ---");
    
    var totalRows = db.getRowCount("test_users");
    print("✓ Total row count: " + totalRows);
    
    var adultRows = db.getRowCount("test_users", {"age": 30});
    print("✓ Adult users (age >= 30): " + adultRows);
    
    // Test Phase 3: DELETE Operations
    print("\n--- Testing DELETE Operations ---");
    
    // Single delete
    var deleteConditions = {
        "email": "charlie@example.com"
    };
    
    var deletedRows = db.deleteRecord("test_users", deleteConditions);
    print("✓ Single delete completed, affected rows: " + deletedRows);
    
    // Batch delete
    var deleteIds = [insertId2];
    var batchDeleteResult = db.deleteBatch("test_users", deleteIds, "id");
    print("✓ Batch delete completed, results: " + batchDeleteResult);
    
    // Test Phase 3: Index Operations
    print("\n--- Testing Index Operations ---");
    
    // Create index
    var indexResult = db.createIndex("test_users", ["email"], "idx_email_unique", true);
    print("✓ Created unique index on email: " + indexResult);
    
    // Create composite index
    var compositeIndexResult = db.createIndex("test_users", ["name", "age"], "idx_name_age", false);
    print("✓ Created composite index: " + compositeIndexResult);
    
    // Verify final state
    print("\n--- Final Verification ---");
    
    var finalUsers = db.select("test_users");
    print("✓ Final user count: " + finalUsers.length);
    
    var finalRowCount = db.getRowCount("test_users");
    print("✓ Final row count verification: " + finalRowCount);
    
    // Test last insert ID and affected rows
    var lastId = db.getLastInsertId();
    print("✓ Last insert ID: " + lastId);
    
    var affectedCount = db.getAffectedRows();
    print("✓ Last affected rows: " + affectedCount);
    
    // Cleanup
    print("\n--- Cleanup ---");
    
    try {
        // Drop indexes
        db.dropIndex("test_users", "idx_email_unique", true);
        print("✓ Dropped unique email index");
        
        db.dropIndex("test_users", "idx_name_age", true);
        print("✓ Dropped composite index");
        
        // Drop table
        db.dropTable("test_users", true);
        print("✓ Dropped test table");
    } catch (e) {
        print("Cleanup warning: " + e.getMessage());
    }
    
    // Disconnect
    db.disconnect();
    print("✓ Database disconnected");
    
    print("\n=== Phase 3 Query Execution Engine Test Completed Successfully ===");
    
} catch (e) {
    print("❌ Test failed with error: " + e.getMessage());
    print("Error details: " + e.toString());
}