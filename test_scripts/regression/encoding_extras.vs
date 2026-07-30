// Roadmap Tier 3: uuid_v4() and INI parse/encode.
string $u = uuid_v4();
printnl(string_length($u));            // 36
printnl(string_substr($u, 14, 1));     // 4   (version nibble)
string $ini = "verbose=1
[db]
host=localhost
port=5432";
auto $cfg = ini_parse($ini);
printnl($cfg->verbose);                // 1
printnl($cfg->db->host, ":", $cfg->db->port);  // localhost:5432
// round-trip through ini_encode then re-parse
auto $again = ini_parse(ini_encode($cfg));
printnl($again->db->port);             // 5432
printnl("done");
