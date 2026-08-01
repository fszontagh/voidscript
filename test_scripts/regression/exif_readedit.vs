// Roadmap/feature: EXIF read + edit via the Exif module (exiv2). Copy the fixture to /tmp
// first (binary-safe) so the committed file stays pristine, then set/save/re-read/remove.
string $src = path_join(path_dirname(path_dirname($argv[0])), "fixtures/sample.jpg");
string $work = "/tmp/vs_exif_reg.jpg";
file_put_contents($work, file_get_contents($src), true);

Exif $e = new Exif();
printnl($e->read($work));                 // true
printnl($e->count());                      // 0 (plain jpeg)
$e->set("Exif.Image.Artist", "Alice");
$e->set("Exif.Image.Software", "VoidScript");
printnl($e->count());                      // 2
$e->save();

Exif $r = new Exif();
$r->read($work);
printnl($r->get("Exif.Image.Artist"));     // Alice
printnl(is_null($r->get("Exif.Image.Model")));  // true
printnl($r->remove("Exif.Image.Software")); // true
$r->save();

Exif $r2 = new Exif();
$r2->read($work);
printnl(is_null($r2->get("Exif.Image.Software"))); // true
printnl($r2->get("Exif.Image.Artist"));    // Alice (survived)
printnl("done");
