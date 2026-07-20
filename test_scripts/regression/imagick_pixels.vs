// Regression: task #9 - the Imagick module had no per-pixel access at all
// (read/write/crop/resize/blur/rotate/flip only), so any pixel-level image work was
// impossible and the original portability report listed it as a blocker.
// Expected: clean exit 0.

string $src = path_join(path_dirname(path_dirname($argv[0])), "fixtures/sample.png");
string $out = "/tmp/voidscript_pixel_test.png";

Imagick $img = new Imagick();
$img->read($src);
printnl($img->getWidth(), "x", $img->getHeight());   // 16x16

// The fixture is a generated gradient: pixel (x,y) is (x*16, y*16, 128).
object $p = $img->getPixel(2, 3);
printnl($p->red, ",", $p->green, ",", $p->blue);     // 32,48,128

// Write a known colour and read it back
$img->setPixel(0, 0, 255, 0, 0);
object $q = $img->getPixel(0, 0);
printnl($q->red, ",", $q->green, ",", $q->blue);     // 255,0,0

// Alpha is readable and writable
$img->setPixel(1, 1, 10, 20, 30, 128);
object $r = $img->getPixel(1, 1);
printnl($r->red, ",", $r->green, ",", $r->blue, ",", $r->alpha);   // 10,20,30,128

// The edit must survive a write/read round trip
$img->write($out);
Imagick $reloaded = new Imagick();
$reloaded->read($out);
object $s = $reloaded->getPixel(0, 0);
printnl($s->red, ",", $s->green, ",", $s->blue);     // 255,0,0

// Out-of-range coordinates are rejected rather than reading rubbish
try {
    object $bad = $img->getPixel(999, 999);
    printnl("NOT REACHED");
} catch (string $e) {
    printnl("out-of-range rejected");
}

file_unlink($out);
printnl("done");
