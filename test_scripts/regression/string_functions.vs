// Regression: task #7 - the String module had only string_length, string_replace and
// string_substr, so scripts resorted to tricks (the documented substring test was
// "replace the needle and see if the length changed").
// Expected: clean exit 0.

printnl(string_to_upper("MiXeD"));              // MIXED
printnl(string_to_lower("MiXeD"));              // mixed
printnl(string_trim("  padded  ") + "|");       // padded|
printnl(string_index_of("hello world", "wor")); // 6
printnl(string_index_of("hello", "zzz"));       // -1
printnl(string_contains("hello", "ell"));       // true
printnl(string_contains("hello", "zzz"));       // false
printnl(string_starts_with("hello", "he"));     // true
printnl(string_ends_with("hello", "lo"));       // true
printnl(string_repeat("ab", 3));                // ababab
printnl(string_reverse("abc"));                 // cba

string[] $parts = string_split("a,b,c", ",");
printnl(sizeof($parts));                        // 3
printnl($parts[1]);                             // b
printnl(string_join($parts, "-"));              // a-b-c

// splitting on a separator that is absent yields the whole string
string[] $one = string_split("abc", ",");
printnl(sizeof($one));                          // 1
printnl($one[0]);                               // abc

// edge cases must not crash
printnl(string_trim(""), "|");                  // |
printnl(string_repeat("x", 0), "|");            // |
printnl(string_index_of("", "a"));              // -1

printnl("done");
