# StableDiffusion module

Wraps [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) as a
VoidScript class: text-to-image, image-to-image (incl. inpaint and controlnet), and
ESRGAN upscaling, writing PNGs to a path and returning the path(s). Sampling progress and
log output are captured per instance and readable from the script.

Opt-in (`BUILD_MODULE_STABLEDIFFUSION`, default OFF): needs a prebuilt
stable-diffusion.cpp and (for GPU) CUDA. A normal VoidScript build ignores it.

## Building the dependency

Must be built with `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` so its static libs link into
this shared module.

```bash
git clone --recursive https://github.com/leejet/stable-diffusion.cpp /data/stable-diffusion.cpp
cd /data/stable-diffusion.cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSD_CUDA=ON \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSD_BUILD_SHARED_LIBS=OFF
cmake --build build -j$(nproc)
```

Pinned/verified against upstream commit `5114672` (Mage-Flow model support; HF-format
Qwen3-VL vision patch-size detection fix). Drop `-DSD_CUDA=ON` for CPU-only.

## Building the module

```bash
cmake -S . -B build -DBUILD_MODULE_STABLEDIFFUSION=ON -DSDCPP_ROOT=/data/stable-diffusion.cpp
cmake --build build -j$(nproc)
```

## API

```voidscript
StableDiffusion $sd = new StableDiffusion();
$sd->loadModel({ string model_path: "/path/sd15.safetensors" });

auto $paths = $sd->txt2img({
    string prompt: "a red apple on a table, photo",
    string negative_prompt: "blurry",
    int width: 512, int height: 512, int steps: 20,
    double cfg_scale: 7.0, int seed: 42,
    string sampler: "euler_a", string output: "/tmp/out.png"   // batch => out.png, out_1.png, ...
});
printnl($paths[0]);

$sd->img2img({ string init_image: "in.png", double strength: 0.6,
               string prompt: "...", string output: "/tmp/i2i.png" });
// + mask_image => inpaint;  + control_image (with control_net_path loaded) => controlnet

// instruction edit with reference images (Mage-Flow-Edit, Kontext, ...)
$sd->txt2img({ string prompt: "change the sign text to 'mage.cpp'",
               ref_images: [ "photo.png" ], double cfg_scale: 4.0,
               string sampler: "euler", string output: "/tmp/edit.png" });

$sd->upscale({ string esrgan_path: "/path/4x.pth", string input: "in.png",
               string output: "in_4x.png", int scale: 4 });

$sd->unload();
```

Each instance holds its own `sd_ctx_t*`, keyed by the framework instance id, so multiple
`StableDiffusion` objects are independent.

### loadModel options (all optional except a model source)

Model sources (one required): `model_path` (a full checkpoint) **or** `diffusion_model_path`.
Extra weights: `vae_path`, `clip_l_path`, `clip_g_path`, `clip_vision_path`, `t5xxl_path`,
`llm_path`, `high_noise_diffusion_model_path`, `control_net_path`, `motion_module_path`,
`photo_maker_path`, `pulid_weights_path`, `embeddings_connectors_path`, `taesd_path`,
`tensor_type_rules`.
Enums (strings): `wtype` (e.g. `q8_0`, `f16`), `rng_type`, `sampler_rng_type`,
`prediction`, `lora_apply_mode`.
Scalars/flags: `n_threads`, `enable_mmap`, `flash_attn`, `diffusion_flash_attn`,
`tae_preview_only`, `diffusion_conv_direct`, `vae_conv_direct`, `force_sdxl_vae_conv_scale`,
`eager_load`, `auto_fit`, `stream_layers`, `max_vram` (GiB budget e.g. `"8"` or `"auto"`),
`backend`, `params_backend`, `split_mode`, `rpc_servers`, `model_args`, `quiet` (suppress
the live log echo to stderr; the log is still captured).

### txt2img / img2img options

Required: `prompt`, `output`. img2img also needs `init_image`.
Common: `negative_prompt`, `width`, `height`, `steps`, `cfg_scale`, `seed` (-1 = random),
`batch_count`, `sampler`, `scheduler`, `clip_skip`.
img2img: `strength`, `mask_image`.
Guidance/sampling: `img_cfg`, `distilled_guidance`, `eta`, `flow_shift`,
`shifted_timestep`, `extra_sample_args`.
ControlNet: `control_image`, `control_strength`.
Reference/edit images: `ref_images` (array of image paths) plus an optional
`ref_image_args` preset string. These feed edit/reference-conditioned models
(e.g. Mage-Flow-Edit `-r`, Kontext, PhotoMaker/PuLID reference sets); each image is
VAE-encoded and sent to the diffusion transformer. Works for both txt2img and img2img.
Tiling: `circular_x`, `circular_y`.

Returns an array of output paths (one per `batch_count`).

### video (video-capable models only)

`$sd->video({ string prompt, string output, int video_frames, int fps, int width, int height,
              int steps, double cfg_scale, ... })` -> array of frame paths (`out_0.png`, ...).
Also accepts `init_image` / `end_image` conditioning, `loras`, `vae_tiling`, `strength`,
`moe_boundary`, `vace_strength`, `circular_x/y`. Errors clearly if the loaded model is
image-only (`sd_ctx_supports_video_generation`).

Status: implemented and the full pipeline runs on a real WAN 2.1 model (text encode ->
VACE context -> diffusion -> VAE). On a 12 GB card the WAN VAE decode of the frames OOMs
for the VACE-1.3B model even with `vae_tiling` / TAESD; a lighter/non-VACE t2v model or
more VRAM completes it. Image generation (small VAE) is unaffected.

### vae_tiling

`vae_tiling: true`, or an object `{ enabled, temporal_tiling, tile_size_x, tile_size_y,
target_overlap, rel_size_x, rel_size_y }`. Keeps the VAE compute buffer small for
high-res / video decode. Accepted by txt2img/img2img/video.

### upscale options

Required: `esrgan_path`, `input`, `output`. Optional: `scale` (default 4), `tile_size`,
`n_threads`. Returns the output path.

### Progress and logs

```voidscript
$sd->clearLog();
$sd->txt2img({ ... });
auto $prog = $sd->getProgress();          // array of { int step, int total, double time }
int  $n    = sizeof($prog);
printnl("last: ", $prog[$n-1]->step, "/", $prog[$n-1]->total);
string $log = $sd->getLog();              // captured model/sampling log text
```

**Live** per-step progress: pass a `progress` option naming a script function. It is
called during generation, once per sampling tick, with a `{ int step, int total, double
time }` object - not retrospective.

```voidscript
function onProgress(object $t) {
    printnl("step ", $t->step, "/", $t->total);   // printed live, during generation
}
$sd->txt2img({ ..., string progress: "onProgress" });
```

The handler runs on the interpreter thread as a normal nested call, so it can read and
update enclosing variables. An error thrown by the handler is swallowed and does not
abort generation. `getProgress()` / `getLog()` remain as a retrospective record. Log
lines stream to stderr live unless `quiet: true`.

## Not yet wired

The flat parameters above cover the common workflows. LoRAs, VAE tiling, video and
`ref_images` (reference/edit conditioning) are wired (above). Still not exposed (need
array / nested-struct plumbing): textual-inversion embeddings arrays, `custom_sigmas`,
the built-in hi-res-fix (`hires`), and the decode cache.

## Test

`test_scripts/integration/stablediffusion_txt2img.vs` does a small real generation
(512x512, 20 steps) and checks progress + log capture. Not part of the unattended ctest
suite. Verified against `v1-5-pruned-emaonly-fp16.safetensors` on an RTX 3060.
