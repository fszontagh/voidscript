// Regression: bug #21 - only plain decimal literals lexed. Hex, binary, octal,
// scientific notation and digit separators were all syntax errors, which is what
// blocked enum_comprehensive.vs (it declares colours as 0xFF0000).
// Expected: clean exit 0.

// hex
printnl(0xFF);                  // 255
printnl(0x00FF00);              // 65280
printnl(0xff);                  // 255 - lowercase digits
printnl(0Xff);                  // 255 - uppercase prefix

// binary
printnl(0b1010);                // 10
printnl(0B1111);                // 15

// octal
printnl(0o17);                  // 15

// scientific notation
printnl(1e3);                   // 1000.000000
printnl(1.5e2);                 // 150.000000
printnl(2e-2);                  // 0.020000

// digit separators, in any base
printnl(1_000_000);             // 1000000
printnl(0xFF_FF);               // 65535

// plain decimals must be unaffected
printnl(42);                    // 42
printnl(3.5);                   // 3.500000
printnl(0);                     // 0

// hex in an expression, the enum_comprehensive.vs use case
enum Color { RED = 0xFF0000, GREEN = 0x00FF00 };
printnl(Color.RED);             // 16711680
printnl(0xF0 | 0x0F);           // 255

printnl("done");
