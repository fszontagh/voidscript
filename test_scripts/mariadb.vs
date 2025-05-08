const string $username = "user";
const string $hostname = "127.0.0.1";
const string $password = "password";
const string $database = "employees";
const string $tablename = $database;

string $limit = "10";

printnl("Argc: ", $argc);

if ($argc == 2) {
    $limit = $argv[1];
}

if (module_exists("MariaDB")) {
    throw_error("MariaDB module not found");
}
for (object $module : module_list()) {
    printnl("Module: ", $module->name);
}

MariaDB $db = new MariaDB();
$db->connect( $hostname, $username, $password, $database);

if ($db == NULL) {
    printnl("Failed to initialize DB!");
}else{
    printnl("Database ok");
}

//$db->close();
const string $query1 = sformat("SELECT * FROM {} LIMIT {}", $tablename, $limit);
const object $result1 = $db->query($query1);

const string $query2 = sformat("SELECT * FROM {} LIMIT {},{}", $tablename, $limit, $limit);
const object $result2 = $db->query($query2);


for (object $item : $result1) {
    format("First name: {} Last name: {}\n", item->first_name, item->last_name);
}
printnl("------");
for (object $item : $result2) {
    format("First name: {} Last name: {}\n", item->first_name, item->last_name);
}

$db->close();