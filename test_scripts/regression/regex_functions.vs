// Roadmap Tier 1: regular expressions (std::regex, ECMAScript). match / search (with
// capture groups) / replace (all, $1 back-refs) / split.
printnl(regex_match("[0-9]+", "abc123"));                       // true
printnl(regex_match("^x", "abc"));                              // false
printnl(json_encode(regex_search("(\\w+)@(\\w+)", "x foo@bar"))); // ["foo@bar","foo","bar"]
printnl(regex_replace("[aeiou]", "hello world", "*"));          // h*ll* w*rld
printnl(json_encode(regex_split("\\s*,\\s*", "a, b ,c,  d")));  // ["a","b","c","d"]
printnl(is_null(regex_search("z+", "abc")));                    // true
printnl("done");
