// Regression: composite was implemented but its registration was commented out, and
// crop/blur/rotate/flip were all broken (they read a __image_id__ key on the object
// that nothing wrote - only read/write/resize/getWidth/getHeight had been migrated to
// an external handle map). That external map was ALSO broken: it keyed on the object's
// serialised contents, and a fresh `new Imagick()` serialises identically, so every
// instance collided onto one image - editing one corrupted the others. All of it now
// keys off a per-object __image_id__, the same fix DateTime got.
// Expected: clean exit 0.

string $fix = path_join(path_dirname(path_dirname($argv[0])), "fixtures/sample.png");

// Two instances of the same file must be independent.
Imagick $a = new Imagick();
$a->read($fix);
Imagick $b = new Imagick();
$b->read($fix);
$b->setPixel(2, 2, 255, 0, 0);
object $ap = $a->getPixel(2, 2);
printnl($ap->red, ",", $ap->green, ",", $ap->blue);     // 32,32,128 - a is untouched

// crop / blur / rotate / flip must run (all were dead before).
Imagick $m = new Imagick();
$m->read($fix);
$m->blur(1.0, 0.5);
$m->rotate(90.0);
$m->flip("vertical");
$m->crop(8, 8, 0, 0);
printnl($m->getWidth(), "x", $m->getHeight());          // 8x8

// composite: paint an overlay solid red and overlay it; a covered pixel must change.
Imagick $base = new Imagick();
$base->read($fix);
Imagick $ov = new Imagick();
$ov->read($fix);
for (int $y = 0; $y < 16; $y++) {
    for (int $x = 0; $x < 16; $x++) {
        $ov->setPixel($x, $y, 255, 0, 0);
    }
}
object $pre = $base->getPixel(5, 5);
printnl($pre->red, ",", $pre->green, ",", $pre->blue);   // 80,80,128 - before composite
$base->composite($ov, 0, 0);
object $post = $base->getPixel(5, 5);
printnl($post->red, ",", $post->green, ",", $post->blue); // 255,0,0 - after

// mode() was implemented but its registration was commented out (dead code).
Imagick $g = new Imagick();
$g->read($fix);
$g->mode("GRAY");
object $gp = $g->getPixel(5, 5);
printnl($gp->red == $gp->green && $gp->green == $gp->blue);   // true - grayscale

printnl("done");
