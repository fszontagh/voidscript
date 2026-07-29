// Regression: Imagick::resize(width, height) preserved the source aspect ratio (fit
// inside), so an off-aspect request silently came back 1-2px short - e.g. resize(512, 769)
// on a 2:3 source returned 512x768, leaving the bottom row uncovered by a later
// compositeMultiply mask. The two-int form must now hit EXACTLY width x height (as PHP's
// Imagick does), with an opt-in bestFit for aspect-preserving, and the geometry-string
// form keeping ImageMagick semantics ("!" forces exact).
// Expected: clean exit 0 with the fixed lines below.

// exact by default (this is the bug: was 512x768)
Imagick $a = new Imagick();
$a->newImage(400, 600, "#808080");
$a->resize(512, 769);
printnl($a->getWidth(), "x", $a->getHeight());   // 512x769

// exact for a square target from a non-square source
Imagick $b = new Imagick();
$b->newImage(400, 600, "#808080");
$b->resize(300, 300);
printnl($b->getWidth(), "x", $b->getHeight());   // 300x300

// bestFit=true opts back into aspect-preserving fit-inside
Imagick $c = new Imagick();
$c->newImage(400, 600, "#808080");
$c->resize(512, 769, true);
printnl($c->getWidth(), "x", $c->getHeight());   // 512x768

// string geometry form: "!" forces exact, plain fits inside
Imagick $d = new Imagick();
$d->newImage(400, 600, "#808080");
$d->resize("512x769!");
printnl($d->getWidth(), "x", $d->getHeight());   // 512x769

Imagick $e = new Imagick();
$e->newImage(400, 600, "#808080");
$e->resize("512x769");
printnl($e->getWidth(), "x", $e->getHeight());   // 512x768

printnl("done");
