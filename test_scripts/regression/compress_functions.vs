// Roadmap Tier 3: gzip compression (Compress module, zlib). gzencode/gzdecode round-trip.
string $text = string_repeat("The quick brown fox. ", 50);   // 1050 bytes, repetitive
string $gz = gzencode($text);
printnl(string_length($gz) < string_length($text));   // true (compressed smaller)
printnl(gzdecode($gz) == $text);                       // true (round-trip)
printnl(gzdecode(gzencode($text, 9)) == $text);        // true (level 9)
printnl(gzdecode(gzencode("")) == "");                 // true (empty)
printnl("done");
