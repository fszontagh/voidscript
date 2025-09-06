// MongoDB Error Handling and Edge Cases Tests
// Tests various error conditions and edge cases

// Test Case 1: Invalid Connection String
// Test with malformed URI
try {
    string $connId = mongoConnect("not_a_valid_uri", "testdb", {});
    printnl("Unexpected success with invalid URI");
} catch (string $e) {
    printnl("Expected error with invalid URI:", $e);
}

// Test Case 2: Connection Timeout
// Test with very short timeout
try {
    string $connId = mongoConnect("mongodb://192.0.2.1:27017", "testdb", {
        int connectionTimeout: 1  // Very short timeout
    });
    printnl("Connection succeeded unexpectedly");
} catch (string $e) {
    printnl("Expected timeout error:", $e);
}

// Test Case 3: Operations on Invalid Connection
// Attempt operations with invalid connection ID
try {
    var $collection = mongoGetCollection("invalid_connection_id", "test");
    printnl("Unexpected success with invalid connection");
} catch (string $e) {
    printnl("Expected error with invalid connection:", $e);
}

// Test Case 4: Valid Connection for Further Tests
string $connId = null;
try {
    $connId = mongoConnect("mongodb://localhost:27017", "testdb", {});
    printnl("Connection established for error tests");
} catch (string $e) {
    printnl("Could not establish connection for tests:", $e);
}

// Skip further tests if connection failed
if ($connId == null) {
    printnl("Skipping remaining tests due to connection failure");
    return;
}

// Test Case 5: Insert Invalid Document
// Attempt to insert null or malformed document
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    mongoInsertOne($collection, null);
    printnl("Unexpected success inserting null document");
} catch (string $e) {
    printnl("Expected error inserting null document:", $e);
}

// Test Case 6: Query Non-existent Collection
// Find in a collection that doesn't exist (should create empty result)
try {
    var $collection = mongoGetCollection($connId, "non_existent_collection");
    var $results = mongoFind($collection, {});
    printnl("Query on non-existent collection result:", $results.length, "documents");
} catch (string $e) {
    printnl("Query non-existent collection error:", $e);
}

// Test Case 7: Update with Invalid Filter
// Update with invalid field name or value
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    mongoUpdateOne($collection, { object invalid: { string bad: "value" } }, { object $set: { string test: "value" } });
    printnl("Unexpected success with invalid filter");
} catch (string $e) {
    printnl("Expected error with invalid filter:", $e);
}

// Test Case 8: Delete with Empty Filter
// Delete operation that might affect all documents inappropriately
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    // Insert test data first
    mongoInsertMany($collection, [
        { string name: "test1", int value: 1 },
        { string name: "test2", int value: 2 }
    ]);

    // Attempt delete with empty filter (should work but be careful in real scenarios)
    var $deleteResult = mongoDeleteMany($collection, {});
    printnl("Deleted with empty filter:", $deleteResult.deletedCount, "documents");

    // Verify deletion
    int $remaining = mongoCount($collection, {});
    printnl("Documents remaining after delete:", $remaining);
} catch (string $e) {
    printnl("Delete with empty filter error:", $e);
}

// Test Case 9: Projection with Invalid Fields
// Query with invalid projection syntax
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    var $results = mongoFind($collection, {}, {
        object projection: { invalid $field: 1 }
    });
    printnl("Unexpected success with invalid projection");
} catch (string $e) {
    printnl("Expected error with invalid projection:", $e);
}

// Test Case 10: Bulk Insert with Mixed Valid/Invalid Data
// Test bulk operations with mixed document types
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    array $mixedData = [
        { string name: "valid1", int value: 100 },
        "invalid_string_document",
        { string name: "valid2", int value: 200 }
    ];
    var $result = mongoInsertMany($collection, $mixedData);
    printnl("Bulk insert with mixed data result:", $result.insertedCount, "inserted");
} catch (string $e) {
    printnl("Expected error with mixed document types:", $e);
}

// Test Case 11: Network Error Simulation
// If connection drops mid-operation (hard to simulate reliably)
// This would typically be tested in integration tests

// Test Case 12: Large Document Handling
// Test with document that might exceed size limits
try {
    var $collection = mongoGetCollection($connId, "test_errors");
    string $largeString = "";
    for (int $i = 0; $i < 1000000; $i += 1) {  // 1MB string
        $largeString += "a";
    }
    object $largeDoc = { string data: $largeString };
    mongoInsertOne($collection, $largeDoc);
    printnl("Large document inserted successfully");
} catch (string $e) {
    printnl("Expected error with large document:", $e);
}

// Test Case 13: Concurrent Operations
// Test potential issues with concurrent access (limited in single-threaded script)
try {
    var $collection = mongoGetCollection($connId, "concurrent_test");
    // Multiple operations in sequence (not truly concurrent)
    for (int $i = 0; $i < 10; $i += 1) {
        object $doc = { string test: "concurrent", int index: $i };
        mongoInsertOne($collection, $doc);
    }
    int $count = mongoCount($collection, { string test: "concurrent" });
    printnl("Concurrent operations result:", $count, "documents");
} catch (string $e) {
    printnl("Concurrent operations error:", $e);
}

// Cleanup
mongoDeleteMany(mongoGetCollection($connId, "test_errors"), {});
mongoDeleteMany(mongoGetCollection($connId, "concurrent_test"), {});
mongoDisconnect($connId);