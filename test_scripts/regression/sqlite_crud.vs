// Roadmap Tier 2: SQLite - zero-config embedded SQL with prepared-statement binding.
// In-memory DB, so it runs unattended (no server). Covers open/exec/query/params/
// lastInsertId/changes, typed columns, NULL storage, and injection-safe binding.
SQLite $db = new SQLite();
$db->open(":memory:");
printnl($db->isOpen());                     // true
$db->exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER, score REAL)");
$db->exec("INSERT INTO users (name, age, score) VALUES (?, ?, ?)", ["Alice", 30, 9.5]);
$db->exec("INSERT INTO users (name, age, score) VALUES (?, ?, ?)", ["Bob", 25, 7.0]);
$db->exec("INSERT INTO users (name, age, score) VALUES (?, ?, ?)", ["Cara", 40, 8.0]);
printnl($db->lastInsertId());               // 3

auto $rows = $db->query("SELECT name, age FROM users WHERE age >= ? ORDER BY age", [30]);
printnl(sizeof($rows));                      // 2
printnl($rows[0]->name, " ", $rows[0]->age); // Alice 30
printnl($rows[1]->name, " ", $rows[1]->age); // Cara 40

int $upd = $db->exec("UPDATE users SET score = ? WHERE name = ?", [10.0, "Bob"]);
printnl($upd);                               // 1

// NULL is stored and round-trips (read via json_encode; a bare ->null-member would throw).
$db->exec("INSERT INTO users (name, age) VALUES (?, ?)", ["Dan", 50]);
auto $d = $db->query("SELECT score FROM users WHERE name = ?", ["Dan"]);
printnl(json_encode($d[0]));                 // {"score":null}

// A quote in a bound value is data, not SQL (injection-safe).
$db->exec("INSERT INTO users (name, age) VALUES (?, ?)", ["O'Brien", 33]);
auto $o = $db->query("SELECT age FROM users WHERE name = ?", ["O'Brien"]);
printnl($o[0]->age);                         // 33

$db->close();
printnl($db->isOpen());                      // false
printnl("done");
