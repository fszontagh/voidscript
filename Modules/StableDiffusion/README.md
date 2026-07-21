# StableDiffusion module

Wraps [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) as a
VoidScript class: text-to-image, image-to-image, and ESRGAN upscaling, writing PNGs to a
path. Generated images are returned as an array of paths.

This module is **opt-in** (`BUILD_MODULE_STABLEDIFFUSION`, default OFF) because it needs a
prebuilt stable-diffusion.cpp and (for GPU) CUDA. A normal VoidScript build ignores it.

## Building the dependency

stable-diffusion.cpp must be built with `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` so its
static libraries link into this shared module.

```bash
git clone --recursive https://github.com/leejet/stable-diffusion.cpp /data/stable-diffusion.cpp
cd /data/stable-diffusion.cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSD_CUDA=ON \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSD_BUILD_SHARED_LIBS=OFF
cmake --build build -j$(nproc)
```

(Pinned and verified against upstream commit `5e4e03c`. Drop `-DSD_CUDA=ON` for a
CPU-only build.)

## Building the module

```bash
cmake -S . -B build -DBUILD_MODULE_STABLEDIFFUSION=ON -DSDCPP_ROOT=/data/stable-diffusion.cpp
cmake --build build -j$(nproc)
```

`SDCPP_ROOT` defaults to `/data/stable-diffusion.cpp`; its `build/` must contain the
`.a` archives.

## Usage

```voidscript
StableDiffusion $sd = new StableDiffusion();

$sd->loadModel({ string model_path: "/path/sd15.safetensors" });
// or a split model: diffusion_model_path + vae_path + clip_l_path + t5xxl_path, etc.
// plus optional: control_net_path, taesd_path, wtype, rng_type, flash_attn,
//                diffusion_flash_attn, stream_layers, max_vram ("8" | "auto"), n_threads

auto $paths = $sd->txt2img({
    string prompt: "a red apple on a table, photo",
    string negative_prompt: "blurry",
    int width: 512, int height: 512, int steps: 20,
    double cfg_scale: 7.0, int seed: 42,
    string sampler: "euler_a", string scheduler: "discrete",
    int batch_count: 1,
    string output: "/tmp/out.png"          // "out.png", "out_1.png", ... for a batch
});
printnl($paths[0]);

$sd->img2img({ string init_image: "in.png", double strength: 0.6,
               string prompt: "...", string output: "/tmp/i2i.png" });
// mask_image + init_image => inpainting; control_image + control_net_path => controlnet

$sd->upscale({ string esrgan_path: "/path/4x.pth", string input: "in.png",
               string output: "in_4x.png", int scale: 4 });

$sd->unload();   // free VRAM; the destructor also frees on scope exit
```

Each instance holds its own `sd_ctx_t*`, keyed by the framework instance id, so multiple
`StableDiffusion` objects are independent.

## Test

`test_scripts/integration/stablediffusion_txt2img.vs` does a small real generation. It
needs a GPU and an SD1.5 checkpoint, so it is not part of the unattended ctest suite.
Verified end to end against `v1-5-pruned-emaonly-fp16.safetensors` on an RTX 3060.
