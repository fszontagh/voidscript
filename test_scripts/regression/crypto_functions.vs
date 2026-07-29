// Roadmap Tier 2 (crypto extras, OpenSSL already linked by the Hash module):
// hmac() and random_bytes(). hmac is checked against a known RFC test vector.
printnl(hmac("sha256", "key", "The quick brown fox jumps over the lazy dog"));
// f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8
printnl(string_length(random_bytes(16)));            // 16
printnl(string_length(hex_encode(random_bytes(8)))); // 16
printnl(random_bytes(0) == "");                      // true
printnl("done");
