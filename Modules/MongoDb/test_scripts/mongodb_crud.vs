// MongoDB CRUD Operations Tests
// Tests basic Create, Read, Update, Delete operations

// Setup test data and connection
$mongo : new MongoDB();
$mongo->connect("mongodb://localhost:27017", "testdb");
$collection : $mongo->getCollection("users");

// Test Case 1: Insert One Document
// Insert a single user document
try {
    object $userDoc = {
        string name: "John Doe",
        int age: 30,
        string email: "john@example.com"
    };
    $mongo->insertOne("users", $userDoc);
    printnl("Inserted user document successfully");
} catch (string $e) {
    printnl("Insert failed:", $e);
}

// Test Case 2: Find Documents
// Query for all documents in the collection
try {
    $results : $mongo->find("users", {});
    printnl("Found documents:", $results.length, "total");

// Query with filter
    $specificUser : $mongo->findOne("users", { string name: "John Doe" });
    if ($specificUser != null) {
        printnl("Found specific user:", $specificUser.name, $specificUser.email);
    } else {
        printnl("User not found");
    }
} catch (string $e) {
    printnl("Find operations failed:", $e);
}

// Test Case 3: Update One Document
// Update the user's age
try {
    $updateResult : $mongo->updateOne("users",
        { string name: "John Doe" },
        { object $set: { name: "John Smith", int age: 31 } }
    );
    printnl("Update result:", $updateResult.modifiedCount, "documents modified");
} catch (string $e) {
    printnl("Update failed:", $e);
}

// Test Case 4: Count Documents
// Count all documents and count with filter
try {
    $totalCount : $mongo->count("users", {});
    $adultCount : $mongo->count("users", { int age: { object $gte: 18 } });
    printnl("Total documents:", $totalCount, "Adults:", $adultCount);
} catch (string $e) {
    printnl("Count operations failed:", $e);
}

// Test Case 5: Delete One Document
// Delete the test user
try {
    $deleteResult : $mongo->deleteOne("users", { string name: "John Smith" });
    printnl("Delete result:", $deleteResult.deletedCount, "documents deleted");
} catch (string $e) {
    printnl("Delete failed:", $e);
}

// Test Case 6: Verify Deletion
// Ensure the document was deleted
try {
    $remaining : $mongo->count("users", {});
    printnl("Remaining documents:", $remaining);
} catch (string $e) {
    printnl("Verification failed:", $e);
}

// Cleanup: Disconnect and cleanup test data
$mongo->disconnect();