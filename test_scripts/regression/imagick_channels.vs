// Feature: per-channel split/recombine (enables lateral chromatic aberration - warp R/B
// channels independently, then recombine). Source #2060a0 = R32 G96 B160.
Imagick $r = new Imagick(); $r->newImage(16, 16, "#2060a0"); $r->extractChannel("red");
Imagick $g = new Imagick(); $g->newImage(16, 16, "#2060a0"); $g->extractChannel("green");
Imagick $b = new Imagick(); $b->newImage(16, 16, "#2060a0"); $b->extractChannel("blue");
object $rp = $r->getPixel(8, 8);
printnl($rp->red, ",", $rp->green, ",", $rp->blue);   // 32,32,32 (grayscale of red)

Imagick $out = new Imagick(); $out->newImage(16, 16, "#000000");
$out->combineChannels($r, $g, $b);
object $op = $out->getPixel(8, 8);
printnl($op->red, ",", $op->green, ",", $op->blue);   // 32,96,160
printnl("done");
