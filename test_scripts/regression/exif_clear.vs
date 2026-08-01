// Feature: Exif full-metadata clear (clearAll wipes EXIF/XMP/IPTC/comment).
string $work = "/tmp/vs_exif_clear_reg.jpg";
file_put_contents($work, file_get_contents(path_join(path_dirname(path_dirname($argv[0])), "fixtures/sample.jpg")), true);
Exif $e = new Exif();
$e->read($work);
$e->set("Exif.Image.Artist", "Alice");
$e->set("Exif.Image.Software", "VoidScript");
$e->save();
Exif $chk = new Exif();
$chk->read($work);
printnl($chk->count());        // 2
$chk->clearAll();
$chk->save();
Exif $after = new Exif();
$after->read($work);
printnl($after->count());      // 0
printnl("done");
