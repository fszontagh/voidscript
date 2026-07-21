// Integration test for the StableDiffusion module. Needs a GPU, the stable-diffusion.cpp
// static libs, and an SD1.5 checkpoint. Not part of the unattended ctest suite.
//
//   voidscript "$PWD/test_scripts/integration/stablediffusion_txt2img.vs"

string $model = "/data/SD_MODELS/checkpoints/v1-5-pruned-emaonly-fp16.safetensors";
string $out   = "/tmp/sd_test.png";

StableDiffusion $sd = new StableDiffusion();
printnl("isLoaded (before): ", $sd->isLoaded());     // false

boolean $ok = $sd->loadModel({ string model_path: $model });
printnl("loadModel: ", $ok);                          // true
printnl("isLoaded (after): ", $sd->isLoaded());       // true

// small + few steps so the test is quick
auto $paths = $sd->txt2img({
    string prompt: "a red apple on a wooden table, photo",
    string negative_prompt: "blurry, low quality",
    int width: 256,
    int height: 256,
    int steps: 8,
    double cfg_scale: 7.0,
    int seed: 42,
    string sampler: "euler_a",
    string output: $out
});

printnl("generated: ", $paths[0]);
printnl("file exists: ", file_exists($out));          // true
printnl("file non-empty: ", string_length(file_get_contents($out)) > 1000);  // true

$sd->unload();
printnl("isLoaded (after unload): ", $sd->isLoaded()); // false
printnl("done");
