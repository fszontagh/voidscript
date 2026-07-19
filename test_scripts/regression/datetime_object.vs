// Regression: bug #24 - the DateTime class stored its timestamp two different ways.
// __construct and year()/hour()/minute()/second() used an external map keyed by the
// object's toString(); day() and month() read a "__timestamp__" field on the object
// itself. Nothing wrote that field, so day()/month() were broken, and the external
// map key was the object's serialised contents - fragile and not identity at all.
//
// Everything now stores and reads __timestamp__ on the object.
// Expected: clean exit 0.

DateTime $dt = new DateTime();

int $y = $dt->year();
int $mo = $dt->month();
int $d = $dt->day();
int $h = $dt->hour();
int $mi = $dt->minute();
int $s = $dt->second();

// Assert plausible ranges rather than exact values, since these are "now".
printnl($y > 2000);             // true
printnl($mo >= 1 && $mo <= 12); // true
printnl($d >= 1 && $d <= 31);   // true
printnl($h >= 0 && $h <= 23);   // true
printnl($mi >= 0 && $mi <= 59); // true
printnl($s >= 0 && $s <= 60);   // true

// Two independent instances must both work - the old toString() key collided for
// objects with identical contents.
DateTime $a = new DateTime();
DateTime $b = new DateTime();
printnl($a->year() == $b->year());   // true

printnl("done");
