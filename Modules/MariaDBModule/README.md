# MariaDBModule

This module provides a `MariaDB` class for connecting to a MariaDB/MySQL database, executing queries, and closing the connection in VoidScript via the MariaDB client library.

## Class: MariaDB

### connect
`MariaDB.connect(host, user, password, database) -> MariaDB`

- `host` (string): The database server hostname or IP.
- `user` (string): The username for authentication.
- `password` (string): The password for authentication.
- `database` (string): The name of the database to connect to.

Returns a `MariaDB` instance on success. Throws an error on failure.

### query
`instance.query(sql) -> object | null`

- `sql` (string): The SQL query to execute.

Returns:
- An object mapping row indices ("0", "1", ...) to row objects (column name to value) for `SELECT` queries.
- `null` for queries that do not return a result set (e.g., `INSERT`, `UPDATE`, `DELETE`).

Throws an error on query failure.

### close
`instance.close() -> null`

Closes the database connection. Returns `null`. Subsequent calls have no effect.

## Examples

Basic usage:
```vs
object $db = MariaDB.connect("localhost", "user", "pass", "mydb");
object $rows = $db.query("SELECT id, name FROM users");
for (string $i in keys($rows)) {
    object $row = $rows[$i];
    printnl("User: " + $row["name"]);
}
$db.close();
```

Handling no-result queries:
```vs
object $db = MariaDB.connect("localhost", "user", "pass", "mydb");
null $res = $db.query("DELETE FROM users WHERE id=1");
printnl("Delete result: " + $res);  # prints null
$db.close();
```

## Integration

Ensure the `MariaDBModule` is registered before running scripts:
```cpp
Modules::ModuleManager::instance().addModule(
    std::make_unique<Modules::MariaDBModule>()
);
Modules::ModuleManager::instance().registerAll();
```

Place this file alongside `CMakeLists.txt` and `src/` in the `Modules/MariaDBModule/` folder.