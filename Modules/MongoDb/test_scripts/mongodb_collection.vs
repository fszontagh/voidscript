// MongoDB Collection Management Tests
// Tests collection creation, dropping, and listing

// Setup connection (without specific database to test database-level operations)
$mongo : new MongoDB();
$mongo->connect("mongodb://localhost:27017", "testdb");

// Test Case 1: Create Collection
// Create a new collection explicitly
try {
    $mongo->createCollection("test_collection");
    printnl("Collection 'test_collection' created successfully");
} catch (string $e) {
    printnl("Create collection failed:", $e);
}

// Test Case 2: Verify Collection Creation
// List collections to confirm creation
try {
    $collections : $mongo->listCollections();
    $collectionExists = false;
    for (object $col : $collections) {
        if ($col.name == "test_collection") {
            $collectionExists = true;
            break;
        }
    }
    printnl("'test_collection' exists:", $collectionExists);
} catch (string $e) {
    printnl("Verify collection failed:", $e);
}

// Test Case 3: Get Collection Object
// Get a reference to the collection for operations
try {
    $collection : $mongo->getCollection("test_collection");
    printnl("Got collection reference successfully");
} catch (string $e) {
    printnl("Get collection failed:", $e);
}

// Test Case 4: Add Data to Collection
// Insert test data to the collection
try {
    if ($collection != null) {
        $testDoc = { string test: "data", int number: 42 };
        $mongo->insertOne("test_collection", $testDoc);
        printnl("Test data inserted into collection");
    }
} catch (string $e) {
    printnl("Insert test data failed:", $e);
}

// Test Case 5: Drop Collection
// Drop the test collection
try {
    $mongo->dropCollection("test_collection");
    printnl("Collection 'test_collection' dropped successfully");
} catch (string $e) {
    printnl("Drop collection failed:", $e);
}

// Test Case 6: Verify Collection Dropped
// List collections again to confirm dropping
try {
    $collectionsAfter : $mongo->listCollections();
    $stillExists = false;
    for (object $col : $collectionsAfter) {
        if ($col.name == "test_collection") {
            $stillExists = true;
            break;
        }
    }
    printnl("'test_collection' still exists after drop:", $stillExists);
} catch (string $e) {
    printnl("Verify drop failed:", $e);
}

// Test Case 7: List All Collections
// List all collections in the database
try {
    $allCollections : $mongo->listCollections();
    printnl("Total collections in database:", $allCollections.length);
    for (object $col : $allCollections) {
        printnl("Collection:", $col.name);
    }
} catch (string $e) {
    printnl("List collections failed:", $e);
}

// Test Case 8: Create Collection with Options
// Create collection with specific options (if supported)
try {
    // Note: Some MongoDB deployments may not support all collection options
    $mongo->createCollection("capped_collection");
    printnl("Capped collection created (if supported)");
} catch (string $e) {
    printnl("Create collection with options failed:", $e);
}

// Test Case 9: Cleanup Test Collections
// Drop any remaining test collections
try {
    $mongo->dropCollection("capped_collection");
    printnl("Cleanup completed for test collections");
} catch (string $e) {
    printnl("Cleanup failed:", $e);
}

// Cleanup connection
$mongo->disconnect();