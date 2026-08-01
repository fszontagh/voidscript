// Feature: Imagick image-image arithmetic (compositeOp), geometric distort, and a
// quality argument on write().
Imagick $a = new Imagick();
$a->newImage(32, 32, "#808080");          // 128
Imagick $sub = new Imagick();
$sub->newImage(32, 32, "#202020");        // 32
$a->compositeOp($sub, "subtract", 0, 0);
object $p = $a->getPixel(5, 5);
printnl($p->red);                          // 96

Imagick $b = new Imagick();
$b->newImage(32, 32, "#101010");          // 16
$b->compositeOp($sub, "add", 0, 0);
object $q = $b->getPixel(5, 5);
printnl($q->red);                          // 48

// distort must run and produce a valid image; write accepts a quality arg
Imagick $d = new Imagick();
$d->newImage(48, 48, "#3050a0");
$d->distort("barrel", [0.0, 0.0, 0.4, 0.0]);
$d->write("/tmp/vs_distort_reg.jpg", 80);
printnl(file_exists("/tmp/vs_distort_reg.jpg"));  // true
printnl("done");
