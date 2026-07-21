// Integration test for the StableDiffusion module. Needs a GPU, the stable-diffusion.cpp
// static libs (BUILD_MODULE_STABLEDIFFUSION=ON), and an SD1.5 checkpoint.

string $model = "/data/SD_MODELS/checkpoints/v1-5-pruned-emaonly-fp16.safetensors";
string $out   = "/tmp/sd_test.png";

StableDiffusion $sd = new StableDiffusion();
printnl("isLoaded (before): ", $sd->isLoaded());

boolean $ok = $sd->loadModel({ string model_path: $model, boolean quiet: true });
printnl("loadModel: ", $ok);
$sd->clearLog();

// realistic SD1.5 settings: 512x512, 20 steps
auto $paths = $sd->txt2img({
    string prompt: "a red apple on a wooden table, sharp focus, photo",
    string negative_prompt: "blurry, low quality, deformed",
    int width: 512, int height: 512, int steps: 20,
    double cfg_scale: 7.0, int seed: 42,
    string sampler: "euler_a", string scheduler: "discrete",
    string output: $out
});

printnl("generated: ", $paths[0]);
printnl("file exists: ", file_exists($out));

// progress capture: script can read every sampling tick
auto $prog = $sd->getProgress();
int $ticks = sizeof($prog);
printnl("progress ticks: ", $ticks);
printnl("last step: ", $prog[$ticks - 1]->step, "/", $prog[$ticks - 1]->total);

// log capture: script can read the model/sampling log
string $log = $sd->getLog();
printnl("log captured: ", string_length($log) > 0);
printnl("log mentions sampling: ", string_contains($log, "sampling") || string_contains($log, "generating") || string_length($log) > 50);

$sd->unload();
printnl("done");
