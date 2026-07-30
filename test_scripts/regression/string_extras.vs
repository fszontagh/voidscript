// Roadmap Tier 3: String extras - string_pad (left/right/both), ucfirst/lcfirst, title.
printnl(string_pad("7", 3, "0", "left"));   // 007
printnl(string_pad("hi", 5, ".", "right")); // hi...
printnl(string_pad("hi", 6, "-", "both"));  // --hi--
printnl(string_pad("toolong", 3));          // toolong (no truncation)
printnl(string_ucfirst("hello"));           // Hello
printnl(string_lcfirst("HELLO"));           // hELLO
printnl(string_title("hello world-foo bar"));// Hello World-Foo Bar
printnl("done");
