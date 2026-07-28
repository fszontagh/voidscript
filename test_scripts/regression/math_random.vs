// Missing-features item 1 (RNG) and item 4 (exp/E): the Math module had no random
// number generation and no exp()/E(). Ported image utilities needed a random filename
// picker, Gaussian film-grain, and exp() for a vignette kernel.
// Expected: clean exit 0 with the fixed lines below (random values are asserted by
// property, not by exact value, since the exact sequence is not portable).

printnl("exp0=", exp(0.0));          // 1.000000
printnl("Eint=", floor(E()));        // 2

// Seeding makes a sequence reproducible.
rand_seed(1);
int $a = rand_int(1, 6);
rand_seed(1);
int $b = rand_int(1, 6);
printnl("reproducible=", $a == $b);  // true

// rand_int stays within the inclusive bounds.
boolean $inrange = true;
for (int $i = 0; $i < 200; $i++) {
    int $r = rand_int(10, 20);
    if ($r < 10 || $r > 20) { $inrange = false; }
}
printnl("inrange=", $inrange);       // true

// rand_double stays in [0, 1).
boolean $unit = true;
for (int $i = 0; $i < 200; $i++) {
    double $d = rand_double();
    if ($d < 0.0 || $d >= 1.0) { $unit = false; }
}
printnl("unit=", $unit);             // true

// rand_normal returns a real number (n == n is false only for NaN).
double $n = rand_normal(0.0, 1.0);
printnl("normal_ok=", $n == $n);     // true

printnl("done");
