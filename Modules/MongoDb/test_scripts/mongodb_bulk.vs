// MongoDB Bulk Operations Tests
// Tests bulk insert, update, and delete operations

// Setup test data and connection
$mongo : new MongoDB();
$mongo->connect("mongodb://localhost:27017", "testdb");
$collection : $mongo->getCollection("products");

// Test Case 1: Bulk Insert
// Insert multiple product documents
try {
    array $products = [
        { string name: "Laptop", int price: 1200, string category: "Electronics" },
        { string name: "Book", int price: 20, string category: "Education" },
        { string name: "Mouse", int price: 25, string category: "Electronics" },
        { string name: "Notebook", int price: 5, string category: "Education" },
        { string name: "Tablet", int price: 300, string category: "Electronics" }
    ];
    $insertResult : $mongo->insertMany("products", $products);
    printnl("Bulk insert result:", $insertResult.insertedCount, "documents inserted");
} catch (string $e) {
    printnl("Bulk insert failed:", $e);
}

// Test Case 2: Bulk Update
// Update multiple electronics products to add discount
try {
    $updateResult : $mongo->updateMany("products",
        { string category: "Electronics" },
        { object $set: { boolean onSale: true, int discountPercent: 10 } }
    );
    printnl("Bulk update result:", $updateResult.modifiedCount, "documents modified");
} catch (string $e) {
    printnl("Bulk update failed:", $e);
}

// Test Case 3: Verify Bulk Updates
// Count documents affected by the update
try {
    $saleCount : $mongo->count("products", { boolean onSale: true });
    printnl("Products on sale:", $saleCount);
} catch (string $e) {
    printnl("Verification failed:", $e);
}

// Test Case 4: Bulk Delete
// Delete all products under $10
try {
    $deleteResult : $mongo->deleteMany("products", { int price: { object $lt: 10 } });
    printnl("Bulk delete result:", $deleteResult.deletedCount, "documents deleted");
} catch (string $e) {
    printnl("Bulk delete failed:", $e);
}

// Test Case 5: Verify Bulk Deletions
// Count remaining documents
try {
    $remaining : $mongo->count("products", {});
    $remainingProducts : $mongo->find("products", {}, { limit: 10 });
    printnl("Remaining documents:", $remaining);
} catch (string $e) {
    printnl("Verification failed:", $e);
}

// Test Case 6: Empty Bulk Operations
// Test bulk operations with empty arrays
try {
    array $emptyArray = [];
    $emptyInsertResult : $mongo->insertMany("products", $emptyArray);
    printnl("Empty bulk insert result:", $emptyInsertResult.insertedCount, "documents");
} catch (string $e) {
    printnl("Empty bulk operations failed:", $e);
}

// Cleanup: Delete all test data and disconnect
try {
    $mongo->deleteMany("products", {});
    printnl("All test data cleaned up");
} catch (string $e) {
    printnl("Cleanup failed:", $e);
}
$mongo.disconnect();