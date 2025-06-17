# MariaDB Module - Complete API Reference

This document provides a comprehensive reference for all methods, classes, and features available in the MariaDB module for VoidScript.

## Table of Contents

- [Class Overview](#class-overview)
- [Connection Management](#connection-management)
- [Query Execution](#query-execution)
- [CRUD Operations](#crud-operations)
- [Transaction Management](#transaction-management)
- [Security Methods](#security-methods)
- [Utility Methods](#utility-methods)
- [Exception Types](#exception-types)
- [Data Types](#data-types)
- [Examples](#examples)

## Class Overview

### MariaDB Class

The main class providing database connectivity and operations.

```voidscript
var db = new MariaDB();
```

**Features:**
- RAII-compliant resource management
- Thread-safe operations
- Automatic connection health monitoring
- Comprehensive error handling
- Security framework integration
- Transaction support with ACID compliance

---

## Connection Management

### connect(host, user, password, database)

Establishes a connection to MariaDB/MySQL server.

**Signature:**
```voidscript
MariaDB connect(string host, string user, string password, string database)
```

**Parameters:**
- `host` (string): Database server hostname or IP address
- `user` (string): Database username for authentication
- `password` (string): Database password for authentication
- `database` (string): Database name to connect to

**Returns:** MariaDB instance (for method chaining)

**Throws:**
- `ConnectionException`: Connection failed or invalid parameters
- `SecurityException`: Invalid connection parameters

**Example:**
```voidscript
var db = new MariaDB();
db.connect("localhost", "myuser", "mypass", "mydatabase");
```

**Enhanced Features:**
- Automatic connection validation
- Health monitoring initialization
- Security parameter validation
- Connection pooling integration

---

### disconnect()

Closes the database connection and releases all resources.

**Signature:**
```voidscript
null disconnect()
```

**Returns:** null

**Example:**
```voidscript
db.disconnect();
```

**Features:**
- Automatic resource cleanup
- Connection health status reset
- Transaction rollback if active
- Memory leak prevention

---

### isConnected()

Checks if the database connection is active and healthy.

**Signature:**
```voidscript
boolean isConnected()
```

**Returns:** boolean - true if connected and healthy

**Example:**
```voidscript
if (db.isConnected()) {
    print("Database is ready");
}
```

**Features:**
- Real-time connection status
- Health verification
- Thread-safe checking

---

### reconnect()

Attempts to re-establish the database connection.

**Signature:**
```voidscript
boolean reconnect()
```

**Returns:** boolean - true if reconnection successful

**Throws:**
- `ConnectionException`: Reconnection failed

**Example:**
```voidscript
if (!db.isConnected()) {
    if (db.reconnect()) {
        print("Reconnected successfully");
    }
}
```

**Features:**
- Automatic connection reinitialization
- Previous connection cleanup
- Health status restoration

---

### getConnectionInfo()

Returns detailed information about the current connection.

**Signature:**
```voidscript
object getConnectionInfo()
```

**Returns:** Object with connection details:
- `connection_id` (string): Unique connection identifier
- `is_connected` (boolean): Connection status
- `is_healthy` (boolean): Health status
- `host` (string): Database host
- `database` (string): Database name

**Example:**
```voidscript
var info = db.getConnectionInfo();
print("Connection ID: " + info.connection_id);
print("Status: " + (info.is_connected ? "Connected" : "Disconnected"));
```

---

## Query Execution

### query(sql)

Executes a raw SQL query. **Use with caution** - prefer parameterized methods.

**Signature:**
```voidscript
object query(string sql)
```

**Parameters:**
- `sql` (string): SQL query to execute

**Returns:** Object with query results or null for non-SELECT queries

**Throws:**
- `QueryException`: Query execution failed
- `SecurityException`: Query contains potential security issues

**Example:**
```voidscript
var result = db.query("SELECT COUNT(*) as total FROM users");
```

**Security Note:** This method performs security validation but parameterized methods are preferred.

---

### executeQuery(query, parameters)

Executes a parameterized query safely with automatic parameter binding.

**Signature:**
```voidscript
object executeQuery(string query, array parameters = [])
```

**Parameters:**
- `query` (string): SQL query with parameter placeholders (?)
- `parameters` (array, optional): Array of parameter values

**Returns:** Object with query results

**Throws:**
- `QueryException`: Query execution failed
- `SecurityException`: Invalid parameters or query

**Example:**
```voidscript
var result = db.executeQuery(
    "SELECT * FROM users WHERE age > ? AND city = ?",
    [25, "New York"]
);
```

**Features:**
- 100% SQL injection prevention
- Automatic parameter type validation
- Prepared statement optimization
- Memory-efficient execution

---

## CRUD Operations

### SELECT Operations

#### select(table, columns, conditions, orderBy, limit, offset)

Performs a SELECT query with advanced filtering and sorting options.

**Signature:**
```voidscript
object select(string table, array columns = ["*"], object conditions = {}, 
              string orderBy = "", integer limit = -1, integer offset = 0)
```

**Parameters:**
- `table` (string): Table name
- `columns` (array, optional): Column names to select (default: all columns)
- `conditions` (object, optional): WHERE conditions as key-value pairs
- `orderBy` (string, optional): ORDER BY clause
- `limit` (integer, optional): Maximum number of rows to return
- `offset` (integer, optional): Number of rows to skip

**Returns:** Object with query results indexed by row number

**Throws:**
- `QueryException`: Query execution failed
- `SecurityException`: Invalid table/column names

**Example:**
```voidscript
var users = db.select("users", 
    ["id", "name", "email"], 
    {"status": "active", "age": 25}, 
    "created_at DESC", 
    10, 
    0
);

// Access results
for (var i = 0; i < Object.keys(users).length; i++) {
    var user = users[i];
    print(user.name + " - " + user.email);
}
```

#### selectOne(table, columns, conditions)

Selects a single row from the table.

**Signature:**
```voidscript
object selectOne(string table, array columns = ["*"], object conditions = {})
```

**Returns:** Object with single row data or null if not found

**Example:**
```voidscript
var user = db.selectOne("users", ["*"], {"id": 123});
if (user && user.current_row) {
    print("User found: " + user.current_row.name);
}
```

#### selectColumn(table, column, conditions)

Selects a single column value from the first matching row.

**Signature:**
```voidscript
string selectColumn(string table, string column, object conditions = {})
```

**Returns:** String value of the column or empty string if not found

**Example:**
```voidscript
var email = db.selectColumn("users", "email", {"id": 123});
```

#### selectScalar(table, column, conditions)

Selects a single scalar value with appropriate type conversion.

**Signature:**
```voidscript
mixed selectScalar(string table, string column, object conditions = {})
```

**Returns:** Value with appropriate type (string, integer, double, boolean)

**Example:**
```voidscript
var userCount = db.selectScalar("users", "COUNT(*)", {});
```

### INSERT Operations

#### insert(table, data)

Inserts a single record into the table.

**Signature:**
```voidscript
integer insert(string table, object data)
```

**Parameters:**
- `table` (string): Table name
- `data` (object): Column-value pairs to insert

**Returns:** Integer - last insert ID (auto-increment value)

**Throws:**
- `QueryException`: Insert failed (duplicate key, constraint violation, etc.)
- `SecurityException`: Invalid table name or data types

**Example:**
```voidscript
var userId = db.insert("users", {
    "name": "John Doe",
    "email": "john@example.com",
    "age": 30,
    "created_at": "NOW()"
});
print("User created with ID: " + userId);
```

#### insertBatch(table, dataArray)

Inserts multiple records in a single efficient operation.

**Signature:**
```voidscript
object insertBatch(string table, array dataArray)
```

**Parameters:**
- `table` (string): Table name
- `dataArray` (array): Array of data objects to insert

**Returns:** Object with batch operation results

**Throws:**
- `QueryException`: Batch insert failed
- `SecurityException`: Invalid data structure

**Example:**
```voidscript
var users = [
    {"name": "Alice", "email": "alice@example.com", "age": 25},
    {"name": "Bob", "email": "bob@example.com", "age": 30},
    {"name": "Carol", "email": "carol@example.com", "age": 28}
];

var results = db.insertBatch("users", users);
print("Inserted " + results.length + " users");
```

**Features:**
- Optimized for bulk operations
- Transaction support
- Automatic validation
- Performance improvement over individual inserts

#### insertAndGetId(table, data)

Inserts a record and explicitly returns the auto-increment ID.

**Signature:**
```voidscript
integer insertAndGetId(string table, object data)
```

**Returns:** Integer - the auto-increment ID of the inserted record

**Example:**
```voidscript
var productId = db.insertAndGetId("products", {
    "name": "New Product",
    "price": 29.99,
    "category": "electronics"
});
```

### UPDATE Operations

#### update(table, data, conditions)

Updates records matching the specified conditions.

**Signature:**
```voidscript
integer update(string table, object data, object conditions)
```

**Parameters:**
- `table` (string): Table name
- `data` (object): Column-value pairs to update
- `conditions` (object): WHERE conditions to match records

**Returns:** Integer - number of affected rows

**Throws:**
- `QueryException`: Update failed
- `SecurityException`: Missing conditions (would update all rows)

**Example:**
```voidscript
var affected = db.update("users", 
    {
        "status": "verified", 
        "updated_at": "NOW()"
    }, 
    {
        "id": 123
    }
);
print("Updated " + affected + " rows");
```

**Safety Features:**
- Requires WHERE conditions to prevent accidental mass updates
- Automatic parameter validation
- Transaction support

#### updateBatch(table, dataArray, keyColumn)

Updates multiple records using a key column for identification.

**Signature:**
```voidscript
object updateBatch(string table, array dataArray, string keyColumn)
```

**Parameters:**
- `table` (string): Table name
- `dataArray` (array): Array of data objects with key column values
- `keyColumn` (string): Column name to use as the update key

**Returns:** Object with batch operation results

**Example:**
```voidscript
var updates = [
    {"id": 1, "status": "active", "last_login": "2024-01-01"},
    {"id": 2, "status": "inactive", "last_login": "2023-12-15"},
    {"id": 3, "status": "active", "last_login": "2024-01-02"}
];

var results = db.updateBatch("users", updates, "id");
```

### DELETE Operations

#### deleteRecord(table, conditions)

Deletes records matching the specified conditions.

**Signature:**
```voidscript
integer deleteRecord(string table, object conditions)
```

**Parameters:**
- `table` (string): Table name
- `conditions` (object): WHERE conditions to match records for deletion

**Returns:** Integer - number of deleted rows

**Throws:**
- `QueryException`: Delete failed
- `SecurityException`: Missing conditions (would delete all rows)

**Example:**
```voidscript
var deleted = db.deleteRecord("users", {
    "status": "inactive",
    "last_login": "< '2023-01-01'"
});
print("Deleted " + deleted + " inactive users");
```

**Safety Features:**
- Requires WHERE conditions to prevent accidental mass deletion
- Transaction support for rollback capability

#### deleteBatch(table, keyValues, keyColumn)

Deletes multiple records by key values.

**Signature:**
```voidscript
object deleteBatch(string table, array keyValues, string keyColumn)
```

**Parameters:**
- `table` (string): Table name
- `keyValues` (array): Array of key values to delete
- `keyColumn` (string): Column name to match against

**Returns:** Object with batch operation results

**Example:**
```voidscript
var userIds = [101, 102, 103, 104];
var results = db.deleteBatch("users", userIds, "id");
```

---

## Transaction Management

### beginTransaction()

Starts a new database transaction.

**Signature:**
```voidscript
boolean beginTransaction()
```

**Returns:** boolean - true if transaction started successfully

**Throws:**
- `TransactionException`: Failed to start transaction

**Example:**
```voidscript
db.beginTransaction();
try {
    // Perform multiple related operations
    db.insert("orders", orderData);
    db.update("inventory", stockUpdate, conditions);
    db.commitTransaction();
} catch (e) {
    db.rollbackTransaction();
    throw e;
}
```

**Features:**
- ACID compliance
- Nested transaction support via savepoints
- Automatic rollback on errors

### commitTransaction()

Commits the current transaction, making all changes permanent.

**Signature:**
```voidscript
boolean commitTransaction()
```

**Returns:** boolean - true if commit successful

**Throws:**
- `TransactionException`: No active transaction or commit failed

**Example:**
```voidscript
if (db.isInTransaction()) {
    db.commitTransaction();
}
```

### rollbackTransaction()

Rolls back the current transaction, undoing all changes.

**Signature:**
```voidscript
boolean rollbackTransaction()
```

**Returns:** boolean - true if rollback successful

**Example:**
```voidscript
try {
    db.beginTransaction();
    // Some operations that might fail
    riskOperation();
    db.commitTransaction();
} catch (e) {
    db.rollbackTransaction();
    print("Transaction rolled back due to: " + e);
}
```

### isInTransaction()

Checks if currently within a transaction.

**Signature:**
```voidscript
boolean isInTransaction()
```

**Returns:** boolean - true if transaction is active

**Example:**
```voidscript
if (db.isInTransaction()) {
    print("Currently in transaction");
} else {
    print("Auto-commit mode");
}
```

### Savepoint Management

#### createSavepoint(name)

Creates a named savepoint within the current transaction.

**Signature:**
```voidscript
string createSavepoint(string name = "")
```

**Parameters:**
- `name` (string, optional): Savepoint name (auto-generated if not provided)

**Returns:** string - actual savepoint name used

**Throws:**
- `TransactionException`: Not in transaction or savepoint creation failed

**Example:**
```voidscript
db.beginTransaction();
var savepoint1 = db.createSavepoint("before_updates");

// Perform some operations
db.update("table1", data1, conditions1);

var savepoint2 = db.createSavepoint("after_table1");

// More operations
db.update("table2", data2, conditions2);

// If something goes wrong, rollback to savepoint1
if (errorCondition) {
    db.rollbackToSavepoint(savepoint1);
}
```

#### rollbackToSavepoint(name)

Rolls back to a specific savepoint, undoing changes made after that point.

**Signature:**
```voidscript
boolean rollbackToSavepoint(string name)
```

**Parameters:**
- `name` (string): Savepoint name to rollback to

**Returns:** boolean - true if rollback successful

**Throws:**
- `TransactionException`: Savepoint not found or rollback failed

#### releaseSavepoint(name)

Releases a savepoint, removing it from the transaction.

**Signature:**
```voidscript
boolean releaseSavepoint(string name)
```

**Parameters:**
- `name` (string): Savepoint name to release

**Returns:** boolean - true if release successful

### Isolation Control

#### setIsolationLevel(level)

Sets the transaction isolation level.

**Signature:**
```voidscript
boolean setIsolationLevel(string level)
```

**Parameters:**
- `level` (string): One of:
  - `"READ_UNCOMMITTED"` - Allows dirty reads
  - `"READ_COMMITTED"` - Prevents dirty reads
  - `"REPEATABLE_READ"` - Prevents dirty and non-repeatable reads (default)
  - `"SERIALIZABLE"` - Full isolation

**Returns:** boolean - true if level set successfully

**Example:**
```voidscript
db.setIsolationLevel("SERIALIZABLE");
db.beginTransaction();
// Perform operations with highest isolation
db.commitTransaction();
```

#### getIsolationLevel()

Gets the current transaction isolation level.

**Signature:**
```voidscript
string getIsolationLevel()
```

**Returns:** string - current isolation level

### Auto-commit Control

#### setAutoCommit(enabled)

Controls auto-commit mode for the connection.

**Signature:**
```voidscript
boolean setAutoCommit(boolean enabled)
```

**Parameters:**
- `enabled` (boolean): true to enable auto-commit, false to disable

**Returns:** boolean - true if setting successful

**Example:**
```voidscript
db.setAutoCommit(false); // Disable auto-commit
// Perform multiple operations
db.setAutoCommit(true);  // Re-enable auto-commit
```

#### getAutoCommit()

Gets the current auto-commit status.

**Signature:**
```voidscript
boolean getAutoCommit()
```

**Returns:** boolean - current auto-commit status

### Advanced Transaction Features

#### detectDeadlock()

Detects if the current transaction is involved in a deadlock.

**Signature:**
```voidscript
boolean detectDeadlock()
```

**Returns:** boolean - true if deadlock detected

**Example:**
```voidscript
if (db.detectDeadlock()) {
    print("Deadlock detected, retrying...");
    db.rollbackTransaction();
    // Implement retry logic
}
```

#### getTransactionStatistics()

Returns comprehensive transaction usage statistics.

**Signature:**
```voidscript
object getTransactionStatistics()
```

**Returns:** Object with statistics:
- `transaction_count` (integer): Total transactions started
- `rollback_count` (integer): Total rollbacks performed
- `deadlock_count` (integer): Total deadlocks detected
- `savepoint_count` (integer): Current active savepoints
- `is_in_transaction` (boolean): Current transaction status
- `auto_commit_enabled` (boolean): Auto-commit status
- `isolation_level` (string): Current isolation level

**Example:**
```voidscript
var stats = db.getTransactionStatistics();
print("Transactions: " + stats.transaction_count);
print("Rollbacks: " + stats.rollback_count);
print("Deadlocks: " + stats.deadlock_count);
```

---

## Security Methods

### validateInput(input, type)

Validates input according to specified security rules.

**Signature:**
```voidscript
boolean validateInput(string input, string type)
```

**Parameters:**
- `input` (string): Input string to validate
- `type` (string): Validation type:
  - `"table_name"` - Database table name
  - `"column_name"` - Database column name
  - `"identifier"` - General SQL identifier
  - `"string"` - General string value
  - `"integer"` - Integer format
  - `"float"` - Floating point format

**Returns:** boolean - true if input is valid

**Example:**
```voidscript
if (db.validateInput(userTableName, "table_name")) {
    // Safe to use in query
    var result = db.select(userTableName, ["*"]);
} else {
    print("Invalid table name provided");
}
```

### prepareStatement(query)

Prepares an SQL statement for repeated execution.

**Signature:**
```voidscript
string prepareStatement(string query)
```

**Parameters:**
- `query` (string): SQL query with parameter placeholders (?)

**Returns:** string - statement key for later execution

**Throws:**
- `SecurityException`: Invalid query or security violation

**Example:**
```voidscript
var stmtKey = db.prepareStatement("SELECT * FROM users WHERE age > ? AND city = ?");
var result1 = db.executePrepared(stmtKey, [25, "New York"]);
var result2 = db.executePrepared(stmtKey, [30, "Los Angeles"]);
```

### executePrepared(statementKey, parameters)

Executes a previously prepared statement with parameters.

**Signature:**
```voidscript
object executePrepared(string statementKey, array parameters = [])
```

**Parameters:**
- `statementKey` (string): Key returned by prepareStatement()
- `parameters` (array, optional): Parameter values

**Returns:** Object with query results

---

## Utility Methods

### escapeString(input)

Escapes a string for safe usage in SQL queries (prefer parameterized queries).

**Signature:**
```voidscript
string escapeString(string input)
```

**Parameters:**
- `input` (string): String to escape

**Returns:** string - escaped string

**Example:**
```voidscript
var safeString = db.escapeString(userInput);
// Note: Parameterized queries are preferred over manual escaping
```

### getLastInsertId()

Gets the auto-increment ID from the last INSERT operation.

**Signature:**
```voidscript
integer getLastInsertId()
```

**Returns:** integer - last insert ID

**Example:**
```voidscript
db.insert("users", userData);
var newUserId = db.getLastInsertId();
```

### getAffectedRows()

Gets the number of rows affected by the last UPDATE, INSERT, or DELETE operation.

**Signature:**
```voidscript
integer getAffectedRows()
```

**Returns:** integer - number of affected rows

**Example:**
```voidscript
db.update("users", updateData, conditions);
var updatedCount = db.getAffectedRows();
print("Updated " + updatedCount + " users");
```

### getRowCount(table, conditions)

Counts rows in a table with optional WHERE conditions.

**Signature:**
```voidscript
integer getRowCount(string table, object conditions = {})
```

**Parameters:**
- `table` (string): Table name
- `conditions` (object, optional): WHERE conditions

**Returns:** integer - row count

**Example:**
```voidscript
var totalUsers = db.getRowCount("users");
var activeUsers = db.getRowCount("users", {"status": "active"});
```

---

## Schema Operations

### createTable(tableName, columns, constraints)

Creates a new database table.

**Signature:**
```voidscript
boolean createTable(string tableName, object columns, array constraints = [])
```

**Parameters:**
- `tableName` (string): Name of the table to create
- `columns` (object): Column definitions (name -> SQL type)
- `constraints` (array, optional): Additional table constraints

**Returns:** boolean - true if table created successfully

**Example:**
```voidscript
var success = db.createTable("products", {
    "id": "INT AUTO_INCREMENT PRIMARY KEY",
    "name": "VARCHAR(255) NOT NULL",
    "price": "DECIMAL(10,2)",
    "category_id": "INT",
    "created_at": "TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
}, [
    "FOREIGN KEY (category_id) REFERENCES categories(id)",
    "INDEX idx_name (name)"
]);
```

### dropTable(tableName, ifExists)

Drops a database table.

**Signature:**
```voidscript
boolean dropTable(string tableName, boolean ifExists = true)
```

**Parameters:**
- `tableName` (string): Name of the table to drop
- `ifExists` (boolean, optional): Add IF EXISTS clause to prevent errors

**Returns:** boolean - true if table dropped successfully

### createIndex(tableName, columns, indexName, unique)

Creates an index on table columns.

**Signature:**
```voidscript
boolean createIndex(string tableName, array columns, string indexName = "", boolean unique = false)
```

**Parameters:**
- `tableName` (string): Table name
- `columns` (array): Column names for the index
- `indexName` (string, optional): Custom index name (auto-generated if empty)
- `unique` (boolean, optional): Create unique index

**Returns:** boolean - true if index created successfully

### dropIndex(tableName, indexName, ifExists)

Drops an index from a table.

**Signature:**
```voidscript
boolean dropIndex(string tableName, string indexName, boolean ifExists = true)
```

**Returns:** boolean - true if index dropped successfully

---

## Exception Types

### DatabaseException

Base exception for all database-related errors.

**Properties:**
- `message` (string): Error description
- `error_code` (integer): Database error code
- `sql_state` (string): SQL state code

### ConnectionException

Exception for connection-related errors.

**Common Causes:**
- Invalid connection parameters
- Network connectivity issues
- Authentication failures
- Database server unavailable

### QueryException

Exception for query execution errors.

**Common Causes:**
- SQL syntax errors
- Table or column doesn't exist
- Constraint violations
- Data type mismatches

### SecurityException

Exception for security-related violations.

**Common Causes:**
- SQL injection attempts
- Invalid table/column names
- Parameter type violations
- Input validation failures

### TransactionException

Exception for transaction-related errors.

**Common Causes:**
- Transaction not active
- Deadlock detection
- Commit/rollback failures
- Savepoint errors

---

## Data Types

### Supported Parameter Types

The module supports the following VoidScript types for parameters:

- **STRING**: Text values (VARCHAR, TEXT, etc.)
- **INTEGER**: Whole numbers (INT, BIGINT, etc.)
- **DOUBLE**: Floating-point numbers (DECIMAL, FLOAT, DOUBLE)
- **BOOLEAN**: True/false values (converted to 1/0)
- **NULL_TYPE**: NULL values

### Type Conversion

Automatic type conversion is performed:

```voidscript
// String to appropriate database type
db.insert("products", {
    "name": "Product Name",        // STRING -> VARCHAR
    "price": 29.99,                // DOUBLE -> DECIMAL
    "quantity": 100,               // INTEGER -> INT
    "available": true,             // BOOLEAN -> TINYINT(1)
    "description": null            // NULL -> NULL
});
```

---

## Examples

### Complete Application Example

```voidscript
// E-commerce order processing system
var db = new MariaDB();

function processOrder(customerId, items, shippingAddress) {
    try {
        // Connect to database
        db.connect("localhost", "ecommerce_user", "secure_password", "ecommerce_db");
        
        if (!db.isConnected()) {
            throw "Failed to connect to database";
        }
        
        // Start transaction for data consistency
        db.beginTransaction();
        
        // Calculate order total
        var total = 0;
        for (var i = 0; i < items.length; i++) {
            total += items[i].price * items[i].quantity;
        }
        
        // Create order record
        var orderId = db.insert("orders", {
            "customer_id": customerId,
            "total_amount": total,
            "status": "processing",
            "shipping_address": shippingAddress,
            "created_at": "NOW()"
        });
        
        print("Order created with ID: " + orderId);
        
        // Create savepoint before inventory updates
        var inventorySavepoint = db.createSavepoint("before_inventory");
        
        // Process each order item
        for (var i = 0; i < items.length; i++) {
            var item = items[i];
            
            // Check inventory availability
            var currentStock = db.selectScalar("products", "stock", {"id": item.product_id});
            
            if (currentStock < item.quantity) {
                // Insufficient stock - rollback to savepoint
                db.rollbackToSavepoint(inventorySavepoint);
                throw "Insufficient stock for product " + item.product_id;
            }
            
            // Create order item
            db.insert("order_items", {
                "order_id": orderId,
                "product_id": item.product_id,
                "quantity": item.quantity,
                "unit_price": item.price,
                "total_price": item.price * item.quantity
            });
            
            // Update inventory
            db.update("products", 
                {"stock": currentStock - item.quantity}, 
                {"id": item.product_id}
            );
        }
        
        // Create invoice
        var invoiceId = db.insert("invoices", {
            "order_id": orderId,
            "amount": total,
            "status": "pending",
            "due_date": "DATE_ADD(NOW(), INTERVAL 30 DAY)"
        });
        
        // Update order status
        db.update("orders", 
            {"status": "confirmed", "invoice_id": invoiceId}, 
            {"id": orderId}
        );
        
        // Commit transaction
        db.commitTransaction();
        
        print("Order processed successfully!");
        print("Order ID: " + orderId);
        print("Invoice ID: " + invoiceId);
        print("Total: $" + total);
        
        return {
            "order_id": orderId,
            "invoice_id": invoiceId,
            "total": total,
            "status": "confirmed"
        };
        
    } catch (e) {
        // Rollback transaction on any error
        if (db.isInTransaction()) {
            db.rollbackTransaction();
        }
        
        print("Order processing failed: " + e);
        throw e;
        
    } finally {
        // Always cleanup connection
        if (db.isConnected()) {
            db.disconnect();
        }
    }
}

// Usage example
var customer = 12345;
var orderItems = [
    {"product_id": 101, "quantity": 2, "price": 15.99},
    {"product_id": 205, "quantity": 1, "price": 29.99},
    {"product_id": 350, "quantity": 3, "price": 8.50}
];
var address = "123 Main St, City, State 12345";

try {
    var result = processOrder(customer, orderItems, address);
    print("Order completed: " + result.order_id);
} catch (e) {
    print("Order failed: " + e);
}
```

### Batch Operations Example

```voidscript
var db = new MariaDB();
db.connect("localhost", "user", "pass", "database");

// Efficient bulk data import
function importCustomers(csvData) {
    try {
        db.beginTransaction();
        
        // Prepare batch data
        var batchData = [];
        for (var i = 0; i < csvData.length; i++) {
            batchData.push({
                "name": csvData[i].name,
                "email": csvData[i].email,
                "phone": csvData[i].phone,
                "address": csvData[i].address,
                "created_at": "NOW()"
            });
        }
        
        // Batch insert - much faster than individual inserts
        var results = db.insertBatch("customers", batchData);
        
        // Create customer profiles in batch
        var profileData = [];
        for (var i = 0; i < batchData.length; i++) {
            profileData.push({
                "customer_id": db.getLastInsertId() - batchData.length + i + 1,
                "preferences": "{}",
                "status": "active"
            });
        }
        
        db.insertBatch("customer_profiles", profileData);
        
        db.commitTransaction();
        
        print("Imported " + results.length + " customers successfully");
        return results.length;
        
    } catch (e) {
        db.rollbackTransaction();
        print("Import failed: " + e);
        return 0;
    }
}

db.disconnect();
```

This API reference provides comprehensive documentation for all features of the MariaDB module. For additional examples and best practices, refer to the README.md and migration guide.