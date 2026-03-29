# MariaDB Module for VoidScript

This module provides essential MariaDB/MySQL database connectivity for VoidScript, offering both legacy function-based API and modern object-oriented interface for basic database operations.

## Overview

The MariaDB module supports two distinct APIs:

1. **Legacy Functions**: Simple function-based interface (`mariadbConnect`, `mariadbQuery`, etc.) for basic database operations
2. **Object-Oriented Interface**: Modern class-based API with `MariaDBConnection` class for structured database access

Both APIs provide the same core functionality:
- Database connection management
- SQL query execution (SELECT and non-SELECT queries)
- Result set handling for SELECT queries
- Error handling and connection status checking
- Access to last insert ID and affected rows count
- **SSL/TLS encrypted connections** (optional)

## SSL/TLS Support

The module supports secure SSL/TLS encrypted connections to MariaDB servers:

- **Enable SSL**: Pass `true` as the optional `useSSL` parameter
- **Disable SSL**: Pass `false` or omit the parameter (default: disabled)
- **Backward Compatible**: Existing code continues to work without changes
- **Server Requirements**: MariaDB server must support SSL connections
- **Certificate Validation**: Uses server's SSL certificate (no custom CA required for basic SSL)

## Installation and Setup

### Prerequisites

- **MariaDB/MySQL Client Library**: Development headers and client library must be installed
- **VoidScript**: Compatible with VoidScript runtime environment
- **C++17 Compiler**: For building the module

### Linux Installation

```bash
# Debian/Ubuntu
sudo apt-get update
sudo apt-get install libmariadb-dev libmariadb3

# CentOS/RHEL
sudo yum install mariadb-devel mariadb-libs

# Arch Linux
sudo pacman -S mariadb-clients mariadb-libs
```

### Building the Module

1. Enable MariaDB module in your build configuration:
   ```cmake
   set(BUILD_MODULE_MARIADB ON)
   ```

2. Build with CMake:
   ```bash
   cmake -DBUILD_MODULE_MARIADB=ON ..
   make
   make install
   ```

3. Verify installation:
   ```bash
   # Check if module was built and installed
   ls -la /usr/local/share/voidscript/Modules/ | grep mariadb
   ```

### Database Setup

Create a database and user for your application:

```sql
-- Create database
CREATE DATABASE myapp_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Create user
CREATE USER 'myapp_user'@'localhost' IDENTIFIED BY 'secure_password';

-- Grant permissions
GRANT ALL PRIVILEGES ON myapp_db.* TO 'myapp_user'@'localhost';

-- Flush privileges
FLUSH PRIVILEGES;
```

## Legacy Functions API

### mariadbConnect
`mariadbConnect(host, username, password, database, [useSSL]) -> boolean`

Establishes connection to MariaDB database.

- `host` (string): Database host (e.g., "localhost")
- `username` (string): Database username
- `password` (string): Database password
- `database` (string): Database name
- `useSSL` (boolean, optional): Enable SSL/TLS encryption (default: false)

Returns `true` if connection successful, `false` otherwise.

### mariadbQuery
`mariadbQuery(sql) -> object|int`

Executes SQL query.

- `sql` (string): SQL query string

Returns:
- For SELECT queries: Object containing result rows
- For non-SELECT queries: Integer count of affected rows

### mariadbDisconnect
`mariadbDisconnect() -> boolean`

Closes database connection.

Returns `true` if disconnection successful.

## Object-Oriented API

### MariaDBConnection Class

The `MariaDBConnection` class provides structured database access with connection management.

#### Constructor

```vs
// Create a new connection instance
$db = new MariaDBConnection();
$db->construct();
```

#### Connection Methods

```vs
// Connect to database (non-SSL)
boolean $connected = $db->connect("localhost", "username", "password", "database");

// Connect to database with SSL
boolean $sslConnected = $db->connect("localhost", "username", "password", "database", true);

// Check connection status
boolean $isConnected = $db->isConnected();

// Disconnect
$db->disconnect();
```

#### Query Methods

```vs
// Execute SELECT query - returns result object
object $result = $db->query("SELECT id, name FROM users WHERE active = 1");

// Execute non-SELECT query - returns affected rows count
int $affected = $db->query("UPDATE users SET last_login = NOW() WHERE id = 123");

// Get last insert ID (for INSERT queries)
int $lastId = $db->getLastInsertId();

// Get affected rows count
int $affectedRows = $db->getAffectedRows();
```

## Usage Examples

### Legacy API Examples

#### Basic Connection and Query

```vs
// Connect to database (non-SSL)
boolean $connected = mariadbConnect("localhost", "myuser", "mypass", "mydb");

if (!$connected) {
    printnl("Failed to connect to database");
    return;
}

printnl("Connected successfully!");
```

#### SSL Connection Example

```vs
// Connect to database with SSL encryption
boolean $sslConnected = mariadbConnect("localhost", "myuser", "mypass", "mydb", true);

if (!$sslConnected) {
    printnl("Failed to connect to database with SSL");
    return;
}

printnl("Connected successfully with SSL encryption!");

// Execute queries over encrypted connection
object $result = mariadbQuery("SELECT id, name FROM users LIMIT 5");

mariadbDisconnect();
printnl("SSL connection closed");
```

// Execute SELECT query
object $result = mariadbQuery("SELECT id, name, email FROM users LIMIT 5");

if ($result) {
    printnl("Found ", sizeof($result), " users:");
    for (string $rowKey : keys($result)) {
        object $row = $result[$rowKey];
        printnl("  User: ", $row["name"], " (", $row["email"], ")");
    }
}

// Execute INSERT query
int $affected = mariadbQuery("INSERT INTO users (name, email) VALUES ('John Doe', 'john@example.com')");
printnl("Inserted ", $affected, " row(s)");

// Disconnect
mariadbDisconnect();
printnl("Disconnected from database");
```

#### Error Handling

```vs
try {
    boolean $connected = mariadbConnect("localhost", "wronguser", "wrongpass", "wrongdb");
    if (!$connected) {
        printnl("Connection failed");
        return;
    }

    // This will fail if table doesn't exist
    object $result = mariadbQuery("SELECT * FROM nonexistent_table");

} catch (string $e) {
    printnl("Database error: ", $e);
} finally {
    mariadbDisconnect();
}
```

### OOP API Examples

#### Basic Connection and CRUD Operations

```vs
// Create database connection
$db = new MariaDBConnection();
$db->construct();

// Connect to database (non-SSL)
boolean $connected = $db->connect("localhost", "myuser", "mypass", "mydb");

if (!$connected) {
    printnl("Failed to connect to database");
    return;
}

printnl("Connected successfully!");

// Connect to database with SSL
boolean $sslConnected = $db->connect("localhost", "myuser", "mypass", "mydb", true);

if (!$sslConnected) {
    printnl("Failed to connect to database with SSL");
    return;
}

printnl("Connected successfully with SSL encryption!");

// CREATE TABLE
int $result = $db->query("
    CREATE TABLE IF NOT EXISTS users (
        id INT AUTO_INCREMENT PRIMARY KEY,
        name VARCHAR(100) NOT NULL,
        email VARCHAR(100) UNIQUE,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
");
printnl("Table creation result: ", $result);

// INSERT data
int $affected = $db->query("INSERT INTO users (name, email) VALUES ('Alice Johnson', 'alice@example.com')");
printnl("Inserted ", $affected, " row(s)");
printnl("Last insert ID: ", $db->getLastInsertId());

// SELECT data
object $users = $db->query("SELECT id, name, email, created_at FROM users ORDER BY id");

if ($users) {
    printnl("Found ", sizeof($users), " users:");
    for (string $rowKey : keys($users)) {
        object $row = $users[$rowKey];
        printnl("  ID: ", $row["id"], ", Name: ", $row["name"], ", Email: ", $row["email"]);
    }
}

// UPDATE data
int $updated = $db->query("UPDATE users SET name = 'Alice Smith' WHERE id = 1");
printnl("Updated ", $updated, " row(s)");
printnl("Affected rows: ", $db->getAffectedRows());

// DELETE data
int $deleted = $db->query("DELETE FROM users WHERE id > 10");
printnl("Deleted ", $deleted, " row(s)");

// Disconnect
$db->disconnect();
printnl("Database connection closed");
```

#### Advanced Query Examples

```vs
$db = new MariaDBConnection();
$db->construct();
$db->connect("localhost", "myuser", "mypass", "mydb");

// Complex SELECT with JOIN
object $orders = $db->query("
    SELECT o.id, o.total, u.name as customer_name, o.created_at
    FROM orders o
    JOIN users u ON o.user_id = u.id
    WHERE o.total > 100
    ORDER BY o.created_at DESC
    LIMIT 10
");

if ($orders) {
    printnl("Recent high-value orders:");
    for (string $rowKey : keys($orders)) {
        object $row = $orders[$rowKey];
        printnl("  Order #", $row["id"], " by ", $row["customer_name"], ": $", $row["total"]);
    }
}

// Batch INSERT
string $batchInsert = "
    INSERT INTO products (name, price, category) VALUES
    ('Laptop', 999.99, 'Electronics'),
    ('Book', 19.99, 'Education'),
    ('Headphones', 79.99, 'Electronics')
";
int $inserted = $db->query($batchInsert);
printnl("Batch inserted ", $inserted, " products");

$db->disconnect();
```

#### Error Handling with OOP

```vs
$db = new MariaDBConnection();
$db->construct();

try {
    boolean $connected = $db->connect("localhost", "wronguser", "wrongpass", "wrongdb");
    if (!$connected) {
        printnl("Connection failed");
        return;
    }

    // This will throw an exception if query fails
    object $result = $db->query("SELECT * FROM nonexistent_table");

} catch (string $e) {
    printnl("Database error: ", $e);
} finally {
    if ($db->isConnected()) {
        $db->disconnect();
    }
}
```

## API Reference

### Legacy Functions

#### mariadbConnect(host, username, password, database, [useSSL])
- **Parameters**:
  - `host` (string): Database host
  - `username` (string): Database username
  - `password` (string): Database password
  - `database` (string): Database name
  - `useSSL` (boolean, optional): Enable SSL/TLS encryption (default: false)
- **Returns**: Boolean indicating success
- **Throws**: DatabaseException on connection failure

#### mariadbQuery(sql)
- **Parameters**: SQL query string
- **Returns**: Object (SELECT results) or integer (affected rows)
- **Throws**: DatabaseException on query failure

#### mariadbDisconnect()
- **Parameters**: None
- **Returns**: Boolean indicating success
- **Throws**: DatabaseException on disconnection failure

### MariaDBConnection Class Methods

#### construct()
- **Description**: Initialize the connection object
- **Parameters**: None
- **Returns**: Self reference

#### connect(host, username, password, database, [useSSL])
- **Description**: Establish database connection
- **Parameters**:
  - `host` (string): Database host
  - `username` (string): Database username
  - `password` (string): Database password
  - `database` (string): Database name
  - `useSSL` (boolean, optional): Enable SSL/TLS encryption (default: false)
- **Returns**: Boolean indicating success
- **Throws**: DatabaseException on connection failure

#### disconnect()
- **Description**: Close database connection
- **Parameters**: None
- **Returns**: Boolean indicating success

#### isConnected()
- **Description**: Check if connection is active
- **Parameters**: None
- **Returns**: Boolean connection status

#### query(sql)
- **Description**: Execute SQL query
- **Parameters**: SQL query string
- **Returns**: Object (SELECT results) or integer (affected rows)
- **Throws**: DatabaseException on query failure

#### getLastInsertId()
- **Description**: Get last auto-increment ID from INSERT query
- **Parameters**: None
- **Returns**: Integer last insert ID

#### getAffectedRows()
- **Description**: Get number of affected rows from last query
- **Parameters**: None
- **Returns**: Integer affected rows count

## Error Handling

The module provides comprehensive error handling for various scenarios:

### Connection Errors
```vs
try {
    boolean $connected = $db->connect("wronghost", "wronguser", "wrongpass", "wrongdb");
} catch (string $e) {
    printnl("Connection error: ", $e);
}
```

### Query Errors
```vs
try {
    object $result = $db->query("SELECT * FROM nonexistent_table");
} catch (string $e) {
    printnl("Query error: ", $e);
}
```

### Network Issues
```vs
try {
    object $result = $db->query("SELECT * FROM large_table");
} catch (string $e) {
    printnl("Network/query error: ", $e);
}
```

## Best Practices

### Connection Management
1. **Always check connection status** before executing queries
2. **Close connections** when done to free resources
3. **Use try-catch blocks** for robust error handling
4. **Validate input data** before building SQL queries

### Query Optimization
1. **Use appropriate indexes** on frequently queried columns
2. **Limit result sets** when retrieving large datasets
3. **Use prepared statements** when available (future enhancement)
4. **Batch operations** for multiple similar queries

### Security
1. **Validate and sanitize** all user inputs
2. **Use parameterized queries** when available
3. **Limit database user privileges** to required operations
4. **Monitor query performance** and access patterns

### Performance
1. **Reuse connections** for multiple operations
2. **Close result sets** after processing
3. **Use appropriate data types** in database schema
4. **Monitor query execution times**

## Testing

Comprehensive test scripts are available in the `test_scripts/` directory:

- `mariadb_connection_test.vs` - Basic connection functionality tests
- `mariadb_query_test.vs` - Query execution and result handling tests
- `mariadb_error_handling_test.vs` - Error scenarios and exception handling
- `mariadb_ssl_test.vs` - SSL connection enable/disable tests
- `mariadb_comprehensive_test.vs` - Complete integration tests

Run these tests to verify functionality after building the module:

```bash
# Run basic connection test
voidscript mariadb_connection_test.vs

# Run SSL connection tests
voidscript mariadb_ssl_test.vs

# Run comprehensive query tests
voidscript mariadb_query_test.vs
```

## Dependencies

- **MariaDB/MySQL Client Library**: Required for database connectivity
  - Debian/Ubuntu: `libmariadb-dev`, `libmariadb3`
  - CentOS/RHEL: `mariadb-devel`, `mariadb-libs`
  - Arch Linux: `mariadb-clients`, `mariadb-libs`

## Limitations

This is a simplified MariaDB module implementation that provides:

- ✅ Basic connection management
- ✅ SQL query execution
- ✅ Result set handling
- ✅ Error handling and reporting
- ✅ Both legacy and OOP interfaces

Future enhancements may include:
- Prepared statements
- Transaction support
- Connection pooling
- Batch operations
- Advanced result set streaming

## Integration

Ensure the `MariaDBModule` is registered before running scripts:

```cpp
Modules::ModuleManager::instance().addModule(
    std::make_unique<Modules::MariaDBModule>()
);
Modules::ModuleManager::instance().registerAll();
```

The module automatically registers both legacy functions and OOP classes when loaded.