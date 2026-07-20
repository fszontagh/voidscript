// Regression: task #6 - base64 and sleep were missing entirely (zero hits across src/
// and Modules/), so scripts had to shell out via process_run.
// Expected: clean exit 0.

string $enc = base64_encode("Hello, VoidScript!");
printnl($enc);                                  // SGVsbG8sIFZvaWRTY3JpcHQh
printnl(base64_decode($enc));                   // Hello, VoidScript!

// round trip, including bytes that need padding
printnl(base64_decode(base64_encode("a")));     // a
printnl(base64_decode(base64_encode("ab")));    // ab
printnl(base64_decode(base64_encode("abc")));   // abc
printnl(base64_encode(""), "|");                // |

// sleep must actually pause, but not for long
int $before = current_unix_timestamp();
usleep(200000);                                 // 0.2s, so this test stays fast
sleep(1);
int $after = current_unix_timestamp();
printnl($after - $before >= 1);                 // true

printnl("done");
