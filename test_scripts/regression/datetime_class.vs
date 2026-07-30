// Roadmap Tier 2: DateTime overhaul. The class was broken - format() mangled the string
// (%Y -> %2026) and add* returned a discarded new object (so instances never changed).
// Now: strftime format, in-place calendar-aware arithmetic, diff, instance independence,
// plus date([fmt[,ts]]) and date_parse(). Run under TZ=UTC for a deterministic result.
DateTime $d = new DateTime(1615811445);           // 2021-03-15 12:30:45 UTC
printnl($d->format("%Y-%m-%d %H:%M:%S"));          // 2021-03-15 12:30:45
printnl($d->year(), "-", $d->month(), "-", $d->day());  // 2021-3-15
printnl($d->timestamp());                          // 1615811445

$d->addDays(10);
printnl($d->format("%Y-%m-%d"));                   // 2021-03-25
$d->addMonths(1);
printnl($d->format("%Y-%m-%d"));                   // 2021-04-25
$d->addYears(2);
printnl($d->year());                               // 2023

// diff in seconds
DateTime $a = new DateTime(1000000);
DateTime $b = new DateTime(1000000);
$b->addHours(2);
printnl($b->diff($a));                             // 7200

// two instances are independent (the old toString-collision bug)
DateTime $x = new DateTime(500);
DateTime $y = new DateTime(500);
$x->addDays(1);
printnl($x->timestamp(), " ", $y->timestamp());    // 86900 500

// free functions: format a timestamp, and parse round-trips under UTC
printnl(date("%Y/%m/%d %H:%M:%S", 1615811445));    // 2021/03/15 12:30:45
printnl(date_parse("2021-03-15 12:30:45", "%Y-%m-%d %H:%M:%S"));  // 1615811445
printnl("done");
