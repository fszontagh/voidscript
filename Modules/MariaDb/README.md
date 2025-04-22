# MariaDBModule

This module provides a `MariaDB` class for connecting to a MariaDB/MySQL database, executing queries, and closing the connection in VoidScript via the MariaDB client library.

## Class: MariaDB

### connect
`MariaDb $db = new MariaDb();`  
`MariaDB->connect(string $host, string $user, string $password, string $database) -> MariaDB`

- `host` (string): The database server hostname or IP.
- `user` (string): The username for authentication.
- `password` (string): The password for authentication.
- `database` (string): The name of the database to connect to.

Returns a `MariaDB` instance on success. Throws an error on failure.

### query
`$db->query(string $sql) -> object | null`

- `sql` (string): The SQL query to execute.

Returns:  

- An `object` `array` row indices to row objects (column name to value) for `SELECT` queries.  
- `null` for queries that do not return a result set (e.g., `INSERT`, `UPDATE`, `DELETE`).

Throws an error on query failure.

### close
`$db->close() -> null`

Closes the database connection. Returns `null`.  
Subsequent calls have no effect.

## Examples

Basic usage:  

```vs

const string $username = "user";
const string $hostname = "127.0.0.1";
const string $password = "password";
const string $database = "database";


// initialize the class
MariaDB $db = new MariaDB();

// connect
$db->connect($hostname, $username, $password, $database);

if ($db == NULL) {
    printnl("Failed to initialize DB!");
}else{
    printnl("Database ok");
}


$db->close();
```
