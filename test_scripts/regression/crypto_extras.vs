// Roadmap remaining crypto (Hash module, OpenSSL): base32 and AES-256-GCM.
printnl(base32_encode("foobar"));            // MZXW6YTBOI====== (RFC 4648 vector)
printnl(base32_decode("MZXW6YTBOI======"));  // foobar
// AES round-trip (IV is random, so assert the decrypted text, not the blob)
string $blob = aes_encrypt("s3cret", "hello world");
printnl(aes_decrypt("s3cret", $blob));        // hello world
printnl(string_length($blob) > 28);           // true
// authenticated: a wrong key must fail, not return garbage
boolean $failed = false;
try {
    aes_decrypt("wrong-key", $blob);
} catch (string $e) {
    $failed = true;
}
printnl($failed);                             // true
printnl("done");
