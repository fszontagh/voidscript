# PixelArt module

Wraps [sdpixel2realpixelart](https://github.com/fszontagh/sdpixel2realpixelart) (the `ppa`
tool) as the VoidScript class `PixelArt`. AI image generators often produce sprites that
look like pixel art from a distance but are blurry and use too many colors up close. This
finds the underlying pixel grid, snaps everything back onto it, collapses each grid cell to
a single pixel and optionally quantizes the palette.

Built on OpenCV (core, imgproc, imgcodecs). On by default; the module skips itself cleanly
if OpenCV or the sources are missing, so a machine without OpenCV still builds VoidScript.

## Building

The tool's library sources are compiled straight into the module `.so`. Point
`PIXELART_ROOT` at a checkout of the repo (needs its `src/`):

```bash
git clone https://github.com/fszontagh/sdpixel2realpixelart /data/aipixel2realpixel
cmake -S . -B build -DBUILD_TESTS=ON -DPIXELART_ROOT=/data/aipixel2realpixel
cmake --build build -j$(nproc)
```

`PIXELART_ROOT` defaults to `/data/aipixel2realpixel`. Disable with
`-DBUILD_MODULE_PIXELART=OFF`.

## API

```voidscript
PixelArt $p = new PixelArt();

// reduce to 16 colors, upscale each output pixel 4x
string $out = $p->convert({
    string input: "sprite.png",
    string output: "sprite_fixed.png",
    int colors: 16,
    int scale_result: 4
});
printnl("wrote: ", $out);

// transparent background, 8 colors, into a directory (name derived from the input stem)
$p->convert({ string input: "sprite.png", string output: "out/",
              int colors: 8, boolean transparent: true });
```

`convert` returns the path actually written.

### Options

Common (mirror the `ppa` CLI flags):

- `input` (required) - source image path.
- `output` - output file or directory (default `"."`). A path with a real extension is
  used as-is; a directory (or extensionless path) gets `<input-stem>_result.png`.
- `colors` - number of colors; `0` (default) skips quantization. (`-c`)
- `scale_result` - scale each output pixel: `0` auto (result ~= input size, default),
  `1` true tiny pixel-art size, `N` upscales N times. (`-s`)
- `initial_upscale` - upscale factor before grid detection (default `2`). (`-u`)
- `pixel_width` - force the logical pixel width; `0` auto-detects (default). (`-w`)
- `transparent` - make the background transparent (default `false`). (`-t`)
- `debug_dir` - write the intermediate images here for inspection. (`--debug-dir`)

Advanced tuning (optional nested objects; omit to use the tool's defaults):

- `mesh: { crop_border_pixels, canny_low, canny_high, closure_kernel_size,
  cluster_threshold, angle_threshold_deg, trim_outlier_fraction,
  hough: { rho, theta_deg, threshold, min_line_len, max_line_gap } }`
- `colors_config: { alpha_threshold, transparency_majority_fraction, bin_size,
  top_colors_limit, thumbnail_w, thumbnail_h }`

The transform is stateless (file in, file out); a `PixelArt` instance holds no state, so one
instance can be reused for any number of conversions.

## Test

`test_scripts/integration/pixelart_convert.vs` runs a real conversion and checks the output
file. Not part of the unattended ctest suite (needs OpenCV and the module built).
