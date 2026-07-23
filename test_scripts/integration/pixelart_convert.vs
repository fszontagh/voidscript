// Integration test for the PixelArt module. Needs OpenCV and the module built
// (BUILD_MODULE_PIXELART=ON, PIXELART_ROOT pointing at a sdpixel2realpixelart checkout).

string $in  = "/data/aipixel2realpixel/build/char.png";
string $out = "/tmp/pixelart_out.png";

PixelArt $p = new PixelArt();

// reduce to 16 colors, scale each output pixel 4x
string $written = $p->convert({
    string input: $in,
    string output: $out,
    int colors: 16,
    int scale_result: 4
});
printnl("written: ", $written);
printnl("exists: ", file_exists($out));

// default settings (auto pixel width, no quantization) into a directory
string $written2 = $p->convert({ string input: $in, string output: "/tmp/pixelart_dir/" });
printnl("written2: ", $written2);
printnl("exists2: ", file_exists($written2));

printnl("done");
