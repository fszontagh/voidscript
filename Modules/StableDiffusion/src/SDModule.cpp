#include "SDModule.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

namespace {

// --- option readers over a VoidScript object (map) argument -----------------------

const Symbols::ObjectMap & optionsOf(FunctionArguments & args, const char * method) {
    if (args.size() < 2 || (args[1] != Symbols::Variables::Type::OBJECT && args[1] != Symbols::Variables::Type::CLASS)) {
        throw std::runtime_error(std::string("StableDiffusion::") + method + " expects an options object");
    }
    return args[1]->get<Symbols::ObjectMap>();
}

std::string optStr(const Symbols::ObjectMap & o, const char * key, const std::string & def = "") {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error(std::string("StableDiffusion: option '") + key + "' must be a string");
    }
    return it->second->get<std::string>();
}

long optInt(const Symbols::ObjectMap & o, const char * key, long def) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error(std::string("StableDiffusion: option '") + key + "' must be an integer");
    }
    return it->second->get<int>();
}

double optNum(const Symbols::ObjectMap & o, const char * key, double def) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    switch (it->second->getType()) {
        case Symbols::Variables::Type::DOUBLE:  return it->second->get<double>();
        case Symbols::Variables::Type::FLOAT:   return static_cast<double>(it->second->get<float>());
        case Symbols::Variables::Type::INTEGER: return static_cast<double>(it->second->get<int>());
        default:
            throw std::runtime_error(std::string("StableDiffusion: option '") + key + "' must be a number");
    }
}

bool optBool(const Symbols::ObjectMap & o, const char * key, bool def) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::BOOLEAN) {
        throw std::runtime_error(std::string("StableDiffusion: option '") + key + "' must be a boolean");
    }
    return it->second->get<bool>();
}

// Load a PNG/JPG from disk into an sd_image_t (RGB, 3 channels). Caller frees .data.
sd_image_t loadImage(const std::string & path) {
    int   w = 0, h = 0, c = 0;
    uint8_t * data = stbi_load(path.c_str(), &w, &h, &c, 3);
    if (!data) {
        throw std::runtime_error("StableDiffusion: cannot read image '" + path + "'");
    }
    sd_image_t img;
    img.width   = static_cast<uint32_t>(w);
    img.height  = static_cast<uint32_t>(h);
    img.channel = 3;
    img.data    = data;
    return img;
}

// Write one generated image to a PNG path.
void writePng(const sd_image_t & img, const std::string & path) {
    if (!img.data) {
        throw std::runtime_error("StableDiffusion: generation produced no image data");
    }
    const int stride = static_cast<int>(img.width) * static_cast<int>(img.channel);
    if (!stbi_write_png(path.c_str(), static_cast<int>(img.width), static_cast<int>(img.height),
                        static_cast<int>(img.channel), img.data, stride)) {
        throw std::runtime_error("StableDiffusion: failed to write PNG '" + path + "'");
    }
}

// "out.png" + index 2 -> "out_2.png". Index 0 keeps the base name.
std::string indexedPath(const std::string & base, int index) {
    if (index == 0) {
        return base;
    }
    const auto dot = base.rfind('.');
    if (dot == std::string::npos) {
        return base + "_" + std::to_string(index);
    }
    return base.substr(0, dot) + "_" + std::to_string(index) + base.substr(dot);
}

}  // namespace

void SDModule::registerFunctions() {
    REGISTER_CLASS(this->name());

    REGISTER_METHOD(this->name(), "__construct", {},
                    [this](FunctionArguments & args) { return this->construct(args); },
                    Symbols::Variables::Type::CLASS, "Create a StableDiffusion instance");

    std::vector<Symbols::FunctionParameterInfo> opts = {
        { "options", Symbols::Variables::Type::OBJECT, "Options object" }
    };

    REGISTER_METHOD(this->name(), "loadModel", opts,
                    [this](FunctionArguments & args) { return this->loadModel(args); },
                    Symbols::Variables::Type::BOOLEAN,
                    "Load a model. Keys: model_path | diffusion_model_path, vae_path, clip_l_path, "
                    "clip_g_path, t5xxl_path, control_net_path, taesd_path, n_threads, wtype, rng_type, "
                    "flash_attn, diffusion_flash_attn, stream_layers, max_vram");

    REGISTER_METHOD(this->name(), "isLoaded", {},
                    [this](FunctionArguments & args) { return this->isLoaded(args); },
                    Symbols::Variables::Type::BOOLEAN, "Whether a model is loaded");

    REGISTER_METHOD(this->name(), "unload", {},
                    [this](FunctionArguments & args) { return this->unload(args); },
                    Symbols::Variables::Type::NULL_TYPE, "Free the loaded model");

    REGISTER_METHOD(this->name(), "txt2img", opts,
                    [this](FunctionArguments & args) { return this->txt2img(args); },
                    Symbols::Variables::Type::OBJECT,
                    "Generate image(s) from a prompt. Keys: prompt (required), output (required), "
                    "negative_prompt, width, height, steps, cfg_scale, seed, sampler, scheduler, "
                    "clip_skip, batch_count, control_image, control_strength. Returns an array of paths.");

    REGISTER_METHOD(this->name(), "img2img", opts,
                    [this](FunctionArguments & args) { return this->img2img(args); },
                    Symbols::Variables::Type::OBJECT,
                    "As txt2img plus init_image (required), strength, mask_image. Returns an array of paths.");

    REGISTER_METHOD(this->name(), "upscale", opts,
                    [this](FunctionArguments & args) { return this->upscale(args); },
                    Symbols::Variables::Type::STRING,
                    "ESRGAN upscale. Keys: esrgan_path (required), input (required), output (required), scale.");
}

Symbols::ValuePtr SDModule::construct(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("StableDiffusion::__construct must be called on a StableDiffusion instance");
    }
    // Stamp the instance id now so the object carries stable identity from the start.
    Symbols::ValuePtr::instanceId(args[0]);
    return args[0];
}

sd_ctx_t * SDModule::contextFor(FunctionArguments & args, const char * method) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error(std::string("StableDiffusion::") + method + " must be called on a StableDiffusion instance");
    }
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = contexts_.find(id);
    if (it == contexts_.end() || it->second == nullptr) {
        throw std::runtime_error(std::string("StableDiffusion::") + method + ": no model loaded - call loadModel() first");
    }
    return it->second;
}

Symbols::ValuePtr SDModule::loadModel(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    const auto & o  = optionsOf(args, "loadModel");

    // Free any previously loaded context on this instance.
    if (auto it = contexts_.find(id); it != contexts_.end() && it->second) {
        free_sd_ctx(it->second);
        contexts_.erase(it);
    }

    sd_ctx_params_t p;
    sd_ctx_params_init(&p);

    // Held for the duration of new_sd_ctx (the struct stores const char* into these).
    const std::string model_path        = optStr(o, "model_path");
    const std::string diffusion_model   = optStr(o, "diffusion_model_path");
    const std::string vae_path          = optStr(o, "vae_path");
    const std::string clip_l_path       = optStr(o, "clip_l_path");
    const std::string clip_g_path       = optStr(o, "clip_g_path");
    const std::string t5xxl_path        = optStr(o, "t5xxl_path");
    const std::string control_net_path  = optStr(o, "control_net_path");
    const std::string taesd_path        = optStr(o, "taesd_path");
    const std::string wtype_str         = optStr(o, "wtype");
    const std::string rng_str           = optStr(o, "rng_type");
    // GiB VRAM budget for segmented offload (e.g. "8" or "auto"); helps on smaller cards.
    const std::string max_vram          = optStr(o, "max_vram");

    if (model_path.empty() && diffusion_model.empty()) {
        throw std::runtime_error("StableDiffusion::loadModel: 'model_path' or 'diffusion_model_path' is required");
    }

    if (!model_path.empty())       p.model_path           = model_path.c_str();
    if (!diffusion_model.empty())  p.diffusion_model_path = diffusion_model.c_str();
    if (!vae_path.empty())         p.vae_path             = vae_path.c_str();
    if (!clip_l_path.empty())      p.clip_l_path          = clip_l_path.c_str();
    if (!clip_g_path.empty())      p.clip_g_path          = clip_g_path.c_str();
    if (!t5xxl_path.empty())       p.t5xxl_path           = t5xxl_path.c_str();
    if (!control_net_path.empty()) p.control_net_path     = control_net_path.c_str();
    if (!taesd_path.empty())       p.taesd_path           = taesd_path.c_str();
    if (!wtype_str.empty())        p.wtype                = str_to_sd_type(wtype_str.c_str());
    if (!rng_str.empty())          p.rng_type             = str_to_rng_type(rng_str.c_str());
    if (!max_vram.empty())         p.max_vram             = max_vram.c_str();

    p.n_threads             = static_cast<int>(optInt(o, "n_threads", p.n_threads));
    p.flash_attn            = optBool(o, "flash_attn", p.flash_attn);
    p.diffusion_flash_attn  = optBool(o, "diffusion_flash_attn", p.diffusion_flash_attn);
    p.stream_layers         = optBool(o, "stream_layers", p.stream_layers);

    sd_ctx_t * ctx = new_sd_ctx(&p);
    if (!ctx) {
        throw std::runtime_error("StableDiffusion::loadModel: failed to load model (check paths and VRAM)");
    }
    contexts_[id] = ctx;
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr SDModule::isLoaded(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = contexts_.find(id);
    return Symbols::ValuePtr(it != contexts_.end() && it->second != nullptr);
}

Symbols::ValuePtr SDModule::unload(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    if (auto it = contexts_.find(id); it != contexts_.end() && it->second) {
        free_sd_ctx(it->second);
        contexts_.erase(it);
    }
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr SDModule::txt2img(FunctionArguments & args) {
    return generate(args, "txt2img", false);
}

Symbols::ValuePtr SDModule::img2img(FunctionArguments & args) {
    return generate(args, "img2img", true);
}

Symbols::ValuePtr SDModule::generate(FunctionArguments & args, const char * method, bool isImg2Img) {
    sd_ctx_t *   ctx = contextFor(args, method);
    const auto & o   = optionsOf(args, method);

    const std::string prompt = optStr(o, "prompt");
    const std::string output = optStr(o, "output");
    if (prompt.empty()) {
        throw std::runtime_error(std::string("StableDiffusion::") + method + ": 'prompt' is required");
    }
    if (output.empty()) {
        throw std::runtime_error(std::string("StableDiffusion::") + method + ": 'output' path is required");
    }

    const std::string negative  = optStr(o, "negative_prompt");
    const std::string sampler   = optStr(o, "sampler");
    const std::string scheduler = optStr(o, "scheduler");

    sd_img_gen_params_t p;
    sd_img_gen_params_init(&p);
    p.prompt          = prompt.c_str();
    p.negative_prompt = negative.c_str();
    p.width           = static_cast<int>(optInt(o, "width", 512));
    p.height          = static_cast<int>(optInt(o, "height", 512));
    p.clip_skip       = static_cast<int>(optInt(o, "clip_skip", p.clip_skip));
    p.seed            = optInt(o, "seed", -1);
    p.batch_count     = static_cast<int>(optInt(o, "batch_count", 1));

    p.sample_params.sample_steps       = static_cast<int>(optInt(o, "steps", 20));
    p.sample_params.guidance.txt_cfg   = static_cast<float>(optNum(o, "cfg_scale", 7.0));
    if (!sampler.empty())   p.sample_params.sample_method = str_to_sample_method(sampler.c_str());
    if (!scheduler.empty()) p.sample_params.scheduler     = str_to_scheduler(scheduler.c_str());

    // Optional control image (controlnet), for both txt2img and img2img.
    sd_image_t control{};
    const std::string control_path = optStr(o, "control_image");
    if (!control_path.empty()) {
        control            = loadImage(control_path);
        p.control_image    = control;
        p.control_strength = static_cast<float>(optNum(o, "control_strength", 0.9));
    }

    // img2img extras.
    sd_image_t init{};
    sd_image_t mask{};
    if (isImg2Img) {
        const std::string init_path = optStr(o, "init_image");
        if (init_path.empty()) {
            throw std::runtime_error("StableDiffusion::img2img: 'init_image' path is required");
        }
        init          = loadImage(init_path);
        p.init_image  = init;
        p.strength    = static_cast<float>(optNum(o, "strength", 0.75));
        const std::string mask_path = optStr(o, "mask_image");
        if (!mask_path.empty()) {
            mask         = loadImage(mask_path);
            p.mask_image = mask;
        }
    }

    sd_image_t * out     = nullptr;
    int          num_out = 0;
    const bool   ok      = generate_image(ctx, &p, &out, &num_out);

    // Free any images we loaded regardless of outcome.
    if (init.data)    { stbi_image_free(init.data); }
    if (mask.data)    { stbi_image_free(mask.data); }
    if (control.data) { stbi_image_free(control.data); }

    if (!ok || !out || num_out <= 0) {
        if (out) { free_sd_images(out, num_out); }
        throw std::runtime_error(std::string("StableDiffusion::") + method + ": generation failed");
    }

    Symbols::ObjectMap paths;
    try {
        for (int i = 0; i < num_out; ++i) {
            const std::string path = indexedPath(output, i);
            writePng(out[i], path);
            paths[std::to_string(i)] = Symbols::ValuePtr(path);
        }
    } catch (...) {
        free_sd_images(out, num_out);
        throw;
    }
    free_sd_images(out, num_out);
    return Symbols::ValuePtr(paths);
}

Symbols::ValuePtr SDModule::upscale(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("StableDiffusion::upscale must be called on a StableDiffusion instance");
    }
    const auto & o = optionsOf(args, "upscale");

    const std::string esrgan = optStr(o, "esrgan_path");
    const std::string input  = optStr(o, "input");
    const std::string output = optStr(o, "output");
    if (esrgan.empty() || input.empty() || output.empty()) {
        throw std::runtime_error("StableDiffusion::upscale requires 'esrgan_path', 'input' and 'output'");
    }
    const int factor    = static_cast<int>(optInt(o, "scale", 4));
    const int n_threads = static_cast<int>(optInt(o, "n_threads", sd_get_num_physical_cores()));

    // new_upscaler_ctx(esrgan_path, direct, n_threads, tile_size, backend, params_backend)
    upscaler_ctx_t * up = new_upscaler_ctx(esrgan.c_str(), false, n_threads, 0, nullptr, nullptr);
    if (!up) {
        throw std::runtime_error("StableDiffusion::upscale: failed to load ESRGAN model '" + esrgan + "'");
    }

    sd_image_t   in      = loadImage(input);
    sd_image_t * out     = nullptr;
    int          num_out = 0;
    const bool   ok      = ::upscale(up, in, static_cast<uint32_t>(factor), &out, &num_out);
    stbi_image_free(in.data);
    free_upscaler_ctx(up);

    if (!ok || !out || num_out <= 0) {
        if (out) { free_sd_images(out, num_out); }
        throw std::runtime_error("StableDiffusion::upscale: upscaling failed");
    }
    try {
        writePng(out[0], output);
    } catch (...) {
        free_sd_images(out, num_out);
        throw;
    }
    free_sd_images(out, num_out);
    return Symbols::ValuePtr(output);
}

}  // namespace Modules
