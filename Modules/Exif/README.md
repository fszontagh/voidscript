# Exif module

Read and edit image EXIF metadata as the VoidScript class `Exif`, backed by
[exiv2](https://exiv2.org). Reads and *writes* individual tags (unlike Imagick, whose EXIF
write support is limited). On by default; needs `libexiv2-dev`, skips if absent.

```voidscript
Exif $e = new Exif();
$e->read("photo.jpg");

auto $all = $e->getAll();                       // { "Exif.Image.Make": "Canon", ... }
printnl($e->get("Exif.Image.Model"));           // one tag (null if absent)
printnl($e->count());                            // number of tags

$e->set("Exif.Image.Copyright", "(c) 2026 me"); // add/overwrite (in memory)
$e->set("Exif.Image.Artist", "Alice");
$e->remove("Exif.Photo.UserComment");           // returns whether it existed
$e->save();                                      // write changes back to photo.jpg

$e->saveAs("photo_tagged.jpg");                  // or copy + write to a new file
```

## Methods

- `read(path)` -> bool - open an image and load its EXIF.
- `getAll()` -> object - every tag as `{ key: value }` (values are exiv2 `toString()` form).
- `get(key)` -> string - one tag by its full key (e.g. `"Exif.Photo.DateTimeOriginal"`),
  null if not present.
- `set(key, value)` - add or overwrite a tag (kept in memory until `save`). Values are set
  as text; best suited to the string tags (Artist, Copyright, ImageDescription, Make,
  Model, Software, DateTime, UserComment, ...).
- `remove(key)` -> bool - erase a tag; `clear()` erases all.
- `count()` -> int - tags currently held.
- `save()` - write the EXIF changes back to the file in place.
- `saveAs(path)` - copy the source image to `path` and write the current EXIF there.

Keys use exiv2's `Family.Group.Tag` form. `getAll()` shows the exact keys present in a file.
Each instance holds its own image, keyed by the framework instance id.
