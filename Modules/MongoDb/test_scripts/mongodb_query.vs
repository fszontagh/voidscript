// MongoDB Advanced Query Tests
// Tests complex queries with filters, projections, sorting, and pagination

// Setup test data
$mongo : new MongoDB();
$mongo->connect("mongodb://localhost:27017", "testdb");
$collection : $mongo->getCollection("employees");

// Insert test data
try {
    array $employees = [
        { string name: "Alice Johnson", string department: "Engineering", int salary: 80000, int age: 28 },
        { string name: "Bob Smith", string department: "Sales", int salary: 60000, int age: 32 },
        { string name: "Carol Davis", string department: "Engineering", int salary: 90000, int age: 35 },
        { string name: "David Wilson", string department: "Marketing", int salary: 55000, int age: 29 },
        { string name: "Eve Brown", string department: "Engineering", int salary: 75000, int age: 26 }
    ];
    $mongo->insertMany("employees", $employees);
    printnl("Test employees inserted:", $employees.length);
} catch (string $e) {
    printnl("Setup failed:", $e);
}

// Test Case 1: Filter Queries
// Find employees in Engineering department
try {
    $engineers : $mongo->find("employees", { string department: "Engineering" });
    printnl("Found engineers:", $engineers.length);
    for (object $emp : $engineers) {
        printnl("Engineer:", $emp.name, "-", $emp.salary);
    }
} catch (string $e) {
    printnl("Filter query failed:", $e);
}

// Test Case 2: Comparison Operators
// Find employees with salary > 70000
try {
    $highEarners : $mongo->find("employees", { int salary: { object $gt: 70000 } });
    printnl("High earners (>70k):", $highEarners.length);
} catch (string $e) {
    printnl("Comparison query failed:", $e);
}

// Test Case 3: Range Queries
// Find employees aged 25-35
try {
    $midAge : $mongo->find("employees", { int age: { object $gte: 25, object $lte: 35 } });
    printnl("Mid-age employees (25-35):", $midAge.length);
} catch (string $e) {
    printnl("Range query failed:", $e);
}

// Test Case 4: Projection
// Find employees but only return name and salary
try {
    $projectionResults : $mongo->find("employees", {}, {
        projection: { string name: 1, int salary: 1, boolean _id: 0 }
    });
    printnl("Projected results:", $projectionResults.length, "fields");
    for (object $emp : $projectionResults) {
        printnl("Projected employee:", $emp.name, "-", $emp.salary);
    }
} catch (string $e) {
    printnl("Projection query failed:", $e);
}

// Test Case 5: Sorting
// Find all employees sorted by salary descending
try {
    $sortedResults : $mongo->find("employees", {}, {
        sort: { int salary: -1 }
    });
    printnl("Sorted by salary desc - top employee:", $sortedResults[0].name, "-", $sortedResults[0].salary);
} catch (string $e) {
    printnl("Sorting query failed:", $e);
}

// Test Case 6: Pagination
// Limit results and skip some
try {
    $firstTwo : $mongo->find("employees", {}, {
        int limit: 2,
        int skip: 0,
        sort: { int age: 1 }
    });
    printnl("First 2 (by age):", $firstTwo[0].name, $firstTwo[1].name);

    $nextTwo : $mongo->find("employees", {}, {
        int limit: 2,
        int skip: 2,
        sort: { int age: 1 }
    });
    printnl("Next 2 (by age):", $nextTwo[0].name, $nextTwo[1].name);
} catch (string $e) {
    printnl("Pagination query failed:", $e);
}

// Test Case 7: Combined Query
// Complex query with filter, sort, projection, and limit
try {
    $complexResults : $mongo->find("employees",
        { string department: "Engineering", int salary: { object $gte: 70000 } },
        {
            projection: { string name: 1, int salary: 1, int age: 1 },
            sort: { int salary: -1 },
            int limit: 10
        }
    );
    printnl("Complex query results:", $complexResults.length);
    for (object $emp : $complexResults) {
        printnl("Result:", $emp.name, "-", $emp.salary, "-", $emp.age);
    }
} catch (string $e) {
    printnl("Complex query failed:", $e);
}

// Cleanup
$mongo->deleteMany("employees", {});
$mongo->disconnect();