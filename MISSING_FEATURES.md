# Missing Features (blocking real-world ports)

Features VoidScript would need to natively replace a set of image/file utility
scripts (the Dolphin service-menu tools in `/data/imageEffect/`). Each entry says
what is missing, why it is required, and a suggested API. Verified against the
build on 2026-07-28.

Ranked by how many scripts each unblocks.

## Status (implemented 2026-07-28)

| # | Feature | Status |
|---|---|---|
| 1 | Random number generation | DONE - `rand_int` / `rand_double` / `rand_normal` / `rand_seed` in Math |
| 2 | Blank/solid canvas + extent | DONE - `Imagick::newImage` / `Imagick::extent` |
| 3 | Native noise | DONE - `Imagick::addNoise` |
| 4 | `exp()` + transcendentals | DONE - `exp` and `E` added (sin/cos/tan/log/log10/PI already existed) |
| 5 | evaluate / mask multiply | DONE - `Imagick::evaluate` / `Imagick::compositeMultiply` |
| 6 | Strip metadata | DONE - `Imagick::stripImage` |
| 7 | ML person segmentation | OUT OF SCOPE - needs a model-inference module; external mask + `composite`/`setPixel` alpha already work |
| 8 | StableDiffusion ControlNet | DONE - `control_net_path` (loadModel) + `control_image` / `control_strength` (txt2img/img2img), plus runtime hot-swap `loadControlNet` / `unloadControlNet` / `hasControlNet` (no checkpoint reload) and IP-Adapter (`ip_adapter_path` + `ip_adapter_image` / `ip_adapter_strength`) |

Regression tests: `test_scripts/regression/math_random.vs`,
`test_scripts/regression/imagick_canvas.vs`.

Original entries below, kept for the rationale and API notes.

---

## 1. Random number generation  (HIGH - unblocks the most)
**Status:** absent. `rand`, `random`, `mt_rand`, `random_int`, `rand_range`, `srand`
all report "Function not found".

**Why required:**
- `renameFileIMG.sh` / `renameFileIMG6.sh` pick a random filename
  `IMG_<10000-99999>` and retry on collision. No RNG = no port (currently worked
  around with `process_run("shuf", ...)`).
- Film-grain noise (`imageNoise.py`, and the noise stage of `imageEffect.py`) needs
  a **normal/Gaussian** distribution, not just uniform.

**Suggested API:**
```
int    rand_int(int min, int max)        // inclusive uniform
double rand_double()                     // [0,1)
double rand_normal(double mean, double stddev)   // Gaussian
void   rand_seed(int seed)               // optional, for reproducibility
```

---

## 2. Imagick: create a blank / solid-color canvas  (+ extent / border)
**Status:** absent. Only `read/write/resize/crop/rotate/flip/blur/composite/
getPixel/setPixel/getWidth/getHeight` exist. There is no way to make a new image
from nothing.

**Why required:**
- `imageResize.php` resizes an image keeping aspect ratio, then **letterbox-pads**
  it to an exact target size on a black background (centered). Padding needs a blank
  black canvas to `composite` the resized image onto. Without canvas creation this
  cannot be done.

**Suggested API:**
```
Imagick::newImage(int width, int height, string colorHex)   // e.g. "#000000"
Imagick::extent(int width, int height, int xOff, int yOff)  // pad/crop to size
```

---

## 3. Imagick: native noise operation
**Status:** absent. `setPixel` exists, but a per-pixel loop over a megapixel image in
the Debug-build interpreter is far too slow to be usable.

**Why required:**
- `imageNoise.py` and the grain stage of `imageEffect.py` add noise to every pixel.
  A single vectorized call is needed; scripting the loop is not viable.

**Suggested API:**
```
Imagick::addNoise(string type, double strength)   // "gaussian" | "uniform" | ...
```

---

## 4. Math: `exp()` and a fuller transcendental set
**Status:** partial. `sqrt` and `pow` work; `exp` is "Function not found".

**Why required:**
- The Gaussian vignette mask in `imageEffect.py` is built from `exp(-x^2 / ...)`.
  Without `exp` the kernel cannot be computed.

**Suggested API:** register `exp`, `log`, `log10`, `sin/cos/tan` (if not already),
plus constants `PI`, `E`.

---

## 5. Imagick: vignette / convolution / per-channel evaluate
**Status:** absent.

**Why required:**
- `imageEffect.py` multiplies each channel by a Gaussian mask and applies a darken
  factor. A native "multiply image by mask" / evaluate / convolution op would do this
  in one call instead of a (too-slow) scripted `getPixel`/`setPixel` loop.

**Suggested API:**
```
Imagick::evaluate(string op, double value)         // "multiply", "add", ...
Imagick::compositeMultiply(Imagick maskImage)
```

---

## 6. Imagick: strip metadata
**Status:** absent.

**Why required:**
- The `removeMetadata` menu action strips all EXIF except orientation. Imagick has no
  strip, so it must stay `process_run("exiftool", ...)`.

**Suggested API:** `Imagick::stripImage()`.

---

## 7. ML person segmentation (or run a segmentation model / accept a mask)
**Status:** absent.

**Why required:**
- `imageBlur.py` uses MediaPipe selfie-segmentation to build a person mask, then blurs
  only the background. `blur(radius, sigma)` already exists, but there is no way to
  produce the mask. Hard blocker.

**Suggested API:** out of scope for a scripting language unless a model-inference
module is added; at minimum, allow loading an external mask PNG and compositing with
per-pixel alpha (which `setPixel` alpha + `composite` partly enable).

---

## 8. StableDiffusion module: ControlNet support
**Status:** the module exposes `loadModel/txt2img/img2img/upscale` only - no
ControlNet conditioning.

**Why required:**
- `image2anime.js` runs SD img2img **conditioned on an OpenPose ControlNet map**.
  Everything else in that pipeline maps to VoidScript (Memcached lock, env, HTTP LLM
  call, Imagick, ESRGAN upscale), but without ControlNet the core step is impossible.

**Suggested API:**
```
StableDiffusion::img2img({ ..., string controlnet_model, string control_image,
                           double control_strength })
```

---

## Cross-cutting: per-pixel performance
`getPixel`/`setPixel` work but the Debug-build interpreter makes megapixel pixel
loops impractically slow. Any pixel-level effect (noise, vignette, blend) needs a
native/vectorized path rather than a scripted loop. This is why items 3 and 5 are
"native op" requests rather than "just loop over setPixel".

---

## What is already sufficient (ports that succeeded)
For reference, these needed nothing new and were ported to `.vs`:
- `openposeLocal.sh` -> `process_run` + `path_*`
- `renameFile.sh` (DSC) -> `string_replace`/`string_trim` + `process_run("date")` +
  binary-safe `file_get_contents`/`file_put_contents`
- jpg<->png -> Imagick `read`/`write` (format by extension)
- rotate/flip -> Imagick `rotate(double)` / `flip("vertical"|"horizontal")`
- `renameFileIMG*.sh` -> ported, but rely on `process_run("shuf")` as an RNG stand-in
  (see item 1).
