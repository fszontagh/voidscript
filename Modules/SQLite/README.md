# SQLite module

Wraps [libsqlite3](https://sqlite.org) as the VoidScript class `SQLite`: a zero-config
embedded SQL database (no server), with prepared statements for safe parameter binding.
On by default; skips itself cleanly if `libsqlite3` is missing.

## Building

Needs `libsqlite3-dev` (Debian/Ubuntu) / `sqlite-devel` (Fedora). Disable with
`-DBUILD_MODULE_SQLITE=OFF`.

## API

```voidscript
SQLite $db = new SQLite();
$db->open("/tmp/app.db");            // a file path, or ":memory:" for an in-memory DB
printnl($db->isOpen());              // true

// exec() runs writes/DDL. With no params it accepts a multi-statement script.
$db->exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)");

// Prepared statements: pass an array of values bound to the ? placeholders.
$db->exec("INSERT INTO users (name, age) VALUES (?, ?)", ["Alice", 30]);
$db->exec("INSERT INTO users (name, age) VALUES (?, ?)", ["Bob", 25]);
int $id = $db->lastInsertId();       // rowid of the last INSERT
printnl($db->changes());             // rows changed by the last statement

// query() returns an array of row objects (column name -> value, typed).
auto $rows = $db->query("SELECT * FROM users WHERE age >= ? ORDER BY age", [26]);
printnl(sizeof($rows));              // 1
printnl($rows[0]->name, " ", $rows[0]->age);   // Alice 30

$db->close();
```

## Methods

- `open(path)` -> bool - open/create a database file (or `":memory:"`). A second `open`
  replaces the previous handle.
- `isOpen()` -> bool.
- `exec(sql [, params])` -> int - run a write/DDL statement; returns the number of changed
  rows. Without `params`, a multi-statement script runs in one call; with `params`, a single
  prepared statement is used.
- `query(sql [, params])` -> array of row objects - run a SELECT. Each row maps column name
  to a typed value (INTEGER -> int, REAL -> double, TEXT/BLOB -> string, NULL -> null).
- `lastInsertId()` -> int - rowid of the most recent INSERT.
- `changes()` -> int - rows changed by the most recent statement.
- `close()` -> null.

Bound parameter types map as: int -> INTEGER, double/float -> REAL, boolean -> 0/1,
null -> NULL, anything else -> its string form as TEXT. Always prefer `?` placeholders +
`params` over string-concatenating values into SQL.

**NULL columns:** a SQL NULL comes back as a null value on the row object. Reading it with
`$row->col` throws (VoidScript evaluates the member access before any `is_null()` guard).
To handle nullable columns, `json_encode($row)` and match the text, or iterate
`for (string $k, auto $v : $row) { if (is_null($v)) ... }`, or `COALESCE(col, default)` in
the SQL.

Each instance owns its own `sqlite3*` handle, keyed by the framework instance id, so
multiple `SQLite` objects are independent.

## Test

`test_scripts/integration/sqlite_crud.vs` exercises open/exec/query/lastInsertId/changes
against an in-memory database.
