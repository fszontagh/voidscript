// Regression: bug #5 - the bitwise operators were absent from the lexer entirely.
// '|' did not even tokenise (Type: UNKNOWN). This is what broke enum_access.vs,
// which combines flag enum values with '|'.
// Expected: clean exit 0.

int $a = 12;                    // 1100
int $b = 10;                    // 1010

printnl($a & $b);               // 8   (1000)
printnl($a | $b);               // 14  (1110)
printnl($a ^ $b);               // 6   (0110)
printnl($a << 2);               // 48
printnl($a >> 2);               // 3
printnl(~$a);                   // -13

// Flag combining, the enum_access.vs use case.
int $read = 1;
int $write = 2;
int $exec = 4;
int $combined = $read | $write | $exec;
printnl($combined);             // 7

// Masking a flag out, and testing one.
printnl($combined & $write);    // 2
printnl(($combined & $exec) == $exec);  // true

// Precedence: '|' must bind looser than '+', and shifts tighter than comparison.
printnl(1 | 2 + 4);             // 7  -> 1 | 6
printnl(1 << 3 > 4);            // true -> 8 > 4

// Logical operators must still lex as && and ||, not as two bitwise ops.
printnl(true && false);         // false
printnl(true || false);         // true

printnl("done");
