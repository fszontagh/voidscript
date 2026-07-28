// Missing-features items 2, 3, 5, 6: the Imagick module could not create an image from
// nothing (newImage/extent), had no native noise (addNoise), no per-channel evaluate or
// mask multiply (evaluate/compositeMultiply), and no metadata strip (stripImage). Each
// was a blocker for porting the image utilities (letterbox pad, film grain, vignette).
// Expected: clean exit 0 with the fixed lines below.

// newImage: a solid mid-grey canvas from nothing (#808080 == 128).
Imagick $c = new Imagick();
$c->newImage(64, 48, "#808080");
printnl($c->getWidth(), "x", $c->getHeight());   // 64x48

// evaluate multiply 0.5 darkens 128 -> 64 (raw-factor semantics).
$c->evaluate("multiply", 0.5);
object $p = $c->getPixel(0, 0);
printnl($p->red);                                // 64

// extent pads to an exact size with a fill colour.
Imagick $e = new Imagick();
$e->newImage(10, 10, "#000000");
$e->extent(20, 20, -5, -5, "#ffffff");
printnl($e->getWidth(), "x", $e->getHeight());   // 20x20

// compositeMultiply: white * 50%-grey mask -> 128.
Imagick $w = new Imagick();
$w->newImage(8, 8, "#ffffff");
Imagick $m = new Imagick();
$m->newImage(8, 8, "#808080");
$w->compositeMultiply($m);
object $q = $w->getPixel(0, 0);
printnl($q->red);                                // 128

// addNoise + stripImage + a write round trip must succeed.
$c->addNoise("gaussian", 1.0);
$c->stripImage();
$c->write("/tmp/vs_imagick_canvas.png");
printnl("written=", file_exists("/tmp/vs_imagick_canvas.png"));   // true

printnl("done");
