// Regression: integer `5 / 0` and `5 % 0` are undefined behaviour in C++ and killed
// the whole interpreter with SIGFPE (exit 136) - an uncatchable signal, no diagnostic.
// They now throw a catchable VoidScript error.
// Expected: clean exit 0.

printnl("start");
try {
    int $a = 5 / 0;
    printnl("NOT REACHED");
} catch (string $e) {
    printnl("div caught");
}
try {
    int $b = 5 % 0;
    printnl("NOT REACHED");
} catch (string $e) {
    printnl("mod caught");
}

// with a variable divisor too
int $zero = 0;
try {
    int $c = 10 / $zero;
    printnl("NOT REACHED");
} catch (string $e) {
    printnl("var div caught");
}

// normal division still works
printnl(10 / 2);                    // 5
printnl(10 % 3);                    // 1
printnl(7.0 / 2.0);                 // 3.500000

printnl("done");
