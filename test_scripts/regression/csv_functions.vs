// Roadmap Tier 2: CSV parse/encode with RFC 4180 quoting (embedded commas, doubled
// quotes) and a round trip, plus a custom delimiter.
string $text = "name,age,city
Alice,30,\"New York\"
\"Bob, Jr.\",25,\"Quote\"\"here\"\"\"";
auto $rows = csv_parse($text);
printnl(sizeof($rows));                    // 3
printnl($rows[1][0], "|", $rows[1][2]);    // Alice|New York
printnl($rows[2][0], "|", $rows[2][2]);    // Bob, Jr.|Quote"here"
auto $again = csv_parse(csv_encode($rows));
printnl($again[2][0], "|", $again[2][2]);  // Bob, Jr.|Quote"here"  (round trip)
auto $semi = csv_parse("a;b;c", ";");
printnl($semi[0][1]);                      // b
printnl("done");
