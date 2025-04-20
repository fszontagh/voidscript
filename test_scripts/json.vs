# JSON Encode/Decode Feature Test
# Define an object with nested data
object $user = {
        string name: "Alice",
        int age: 30,
        boolean active: true,
        object prefs: {
                string theme: "dark",
                boolean notifications: false
            }
    };

// Encode to JSON string
string $json = json_encode($user);
printnl("Encoded JSON: ", $json);

// Decode back to object
object $parsed = json_decode($json);
// Re-encode to verify round-trip
string $json2 = json_encode($parsed);
printnl("Re-encoded JSON: ", $json2);
// --- Simple value tests ---
// Integer
int $num = 42;
string $num_json = json_encode($num);
printnl("Encoded integer: ", $num_json);
int $num_decoded = json_decode($num_json);
printnl("Decoded integer: ", $num_decoded);

// String
string $str = "Hello, VoidScript!";
string $str_json = json_encode($str);
printnl("Encoded string: ", $str_json);
string $str_decoded = json_decode($str_json);
printnl("Decoded string: ", $str_decoded);

// Boolean
boolean $flag = true;
string $flag_json = json_encode($flag);
printnl("Encoded boolean: ", $flag_json);
boolean $flag_decoded = json_decode($flag_json);
printnl("Decoded boolean: ", $flag_decoded);

// Double
double $pi = 3.14159;
string $pi_json = json_encode($pi);
printnl("Encoded double: ", $pi_json);
double $pi_decoded = json_decode($pi_json);
printnl("Decoded double: ", $pi_decoded);