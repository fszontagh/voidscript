#include "SDModule.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cstdint>
#include <cstdio>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

namespace {

// --- module-global callback routing ------------------------------------------------
// sd.cpp's log/progress callbacks are process-global, not per-context. Generation is
// synchronous (one generate_image at a time), so a single "currently capturing" target
// is enough. Guarded by a mutex because sd.cpp may log from a worker thread.
std::mutex  g_cbMutex;
SDModule *  g_activeModule   = nullptr;
long        g_activeId       = 0;
bool        g_quiet          = false;
bool        g_cbInstalled    = false;
std::string g_progressHandler;  // script function name to call per progress tick, if any

void logTrampoline(sd_log_level_t /*level*/, const char * text, void * /*data*/) {
    if (!text) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_cbMutex);
    if (!g_quiet) {
        std::fputs(text, stderr);  // preserve sd.cpp's live console output
    }
    if (g_activeModule) {
        g_activeModule->appendLog(text);
    }
}

void progressTrampoline(int step, int steps, float time, void * /*data*/) {
    std::string handler;
    {
        std::lock_guard<std::mutex> lock(g_cbMutex);
        if (g_activeModule) {
            g_activeModule->appendProgress(step, steps, time);
        }
        handler = g_progressHandler;
    }
    // Invoke the script handler OUTSIDE the lock: it may itself log (-> logTrampoline,
    // which re-locks g_cbMutex) and std::mutex is not recursive. The callback fires on
    // the interpreter thread, so callUserFunction is a safe nested call.
    if (!handler.empty() && Interpreter::Interpreter::hasCurrent()) {
        Symbols::ObjectMap tick;
        tick["step"]  = Symbols::ValuePtr(step);
        tick["total"] = Symbols::ValuePtr(steps);
        tick["time"]  = Symbols::ValuePtr(static_cast<double>(time));
        try {
            Interpreter::Interpreter::callUserFunction(handler, { Symbols::ValuePtr(tick) });
        } catch (...) {
            // A handler error must not abort generation.
        }
    }
}

// --- option readers over a VoidScript object (map) argument ------------------------

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

// Read a `loras` option into an sd_lora_t vector. Accepts an array of strings (paths,
// multiplier 1.0) or of objects { path, multiplier, is_high_noise }. Paths are kept
// alive in `keep` for as long as the sd_lora_t pointers are used.
void readLoras(const Symbols::ObjectMap & o, std::list<std::string> & keep, std::vector<sd_lora_t> & out) {
    auto it = o.find("loras");
    if (it == o.end() || it->second->is_null()) {
        return;
    }
    if (it->second->getType() != Symbols::Variables::Type::OBJECT &&
        it->second->getType() != Symbols::Variables::Type::CLASS) {
        throw std::runtime_error("StableDiffusion: option 'loras' must be an array");
    }
    const Symbols::ObjectMap & arr = it->second->get<Symbols::ObjectMap>();
    for (size_t i = 0;; ++i) {
        auto e = arr.find(std::to_string(i));
        if (e == arr.end()) {
            break;
        }
        sd_lora_t lora{};
        lora.multiplier = 1.0f;
        if (e->second->getType() == Symbols::Variables::Type::STRING) {
            keep.push_back(e->second->get<std::string>());
            lora.path = keep.back().c_str();
        } else if (e->second->getType() == Symbols::Variables::Type::OBJECT ||
                   e->second->getType() == Symbols::Variables::Type::CLASS) {
            const Symbols::ObjectMap & lo   = e->second->get<Symbols::ObjectMap>();
            const std::string          path = optStr(lo, "path");
            if (path.empty()) {
                throw std::runtime_error("StableDiffusion: each lora needs a 'path'");
            }
            keep.push_back(path);
            lora.path         = keep.back().c_str();
            lora.multiplier   = static_cast<float>(optNum(lo, "multiplier", 1.0));
            lora.is_high_noise = optBool(lo, "is_high_noise", false);
        } else {
            throw std::runtime_error("StableDiffusion: 'loras' entries must be a path string or an object");
        }
        out.push_back(lora);
    }
}

// Read a `ref_images` option into a vector of loaded sd_image_t (reference/edit images,
// e.g. Mage `-r` edit, Kontext, PhotoMaker/PuLID reference sets). Accepts an array of
// path strings. Each returned image owns stbi-allocated data that the caller must
// stbi_image_free after generate_image returns. An optional `ref_image_args` string is
// copied into `keep` (kept alive for the call) and its c_str() stored via argsOut.
void readRefImages(const Symbols::ObjectMap & o, std::vector<sd_image_t> & out,
                   std::list<std::string> & keep, const char ** argsOut) {
    auto it = o.find("ref_images");
    if (it != o.end() && !it->second->is_null()) {
        if (it->second->getType() != Symbols::Variables::Type::OBJECT &&
            it->second->getType() != Symbols::Variables::Type::CLASS) {
            throw std::runtime_error("StableDiffusion: option 'ref_images' must be an array of paths");
        }
        const Symbols::ObjectMap & arr = it->second->get<Symbols::ObjectMap>();
        for (size_t i = 0;; ++i) {
            auto e = arr.find(std::to_string(i));
            if (e == arr.end()) {
                break;
            }
            if (e->second->getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("StableDiffusion: 'ref_images' entries must be path strings");
            }
            try {
                out.push_back(loadImage(e->second->get<std::string>()));
            } catch (...) {
                for (auto & im : out) {
                    if (im.data) { stbi_image_free(im.data); }
                }
                out.clear();
                throw;
            }
        }
    }
    auto a = o.find("ref_image_args");
    if (a != o.end() && !a->second->is_null() &&
        a->second->getType() == Symbols::Variables::Type::STRING) {
        keep.push_back(a->second->get<std::string>());
        *argsOut = keep.back().c_str();
    }
}

// Read a `vae_tiling` option into an sd_tiling_params_t. Accepts a boolean (enable with
// defaults) or an object { enabled, temporal_tiling, tile_size_x, tile_size_y,
// target_overlap, rel_size_x, rel_size_y }. VAE tiling keeps the VAE compute buffer
// small, which is what fits high-res / video decode into limited VRAM.
void readTiling(const Symbols::ObjectMap & o, const char * key, sd_tiling_params_t & t, std::list<std::string> & keep) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return;
    }
    if (it->second->getType() == Symbols::Variables::Type::BOOLEAN) {
        t.enabled = it->second->get<bool>();
        return;
    }
    if (it->second->getType() != Symbols::Variables::Type::OBJECT &&
        it->second->getType() != Symbols::Variables::Type::CLASS) {
        throw std::runtime_error("StableDiffusion: option 'vae_tiling' must be a boolean or an object");
    }
    const Symbols::ObjectMap & to = it->second->get<Symbols::ObjectMap>();
    t.enabled         = optBool(to, "enabled", true);
    t.temporal_tiling = optBool(to, "temporal_tiling", t.temporal_tiling);
    t.tile_size_x     = static_cast<int>(optInt(to, "tile_size_x", t.tile_size_x));
    t.tile_size_y     = static_cast<int>(optInt(to, "tile_size_y", t.tile_size_y));
    t.target_overlap  = static_cast<float>(optNum(to, "target_overlap", t.target_overlap));
    t.rel_size_x      = static_cast<float>(optNum(to, "rel_size_x", t.rel_size_x));
    t.rel_size_y      = static_cast<float>(optNum(to, "rel_size_y", t.rel_size_y));
    const std::string extra = optStr(to, "extra_tiling_args");
    if (!extra.empty()) {
        keep.push_back(extra);
        t.extra_tiling_args = keep.back().c_str();
    }
}

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

void SDModule::appendLog(const char * text) { logs_[g_activeId] += text; }

void SDModule::appendProgress(int step, int steps, float time) {
    progress_[g_activeId].push_back(ProgressRec{ step, steps, time });
}

void SDModule::beginCapture(long id, const std::string & progressHandler) {
    std::lock_guard<std::mutex> lock(g_cbMutex);
    if (!g_cbInstalled) {
        sd_set_log_callback(logTrampoline, nullptr);
        sd_set_progress_callback(progressTrampoline, nullptr);
        g_cbInstalled = true;
    }
    g_activeModule    = this;
    g_activeId        = id;
    g_progressHandler = progressHandler;
}

void SDModule::endCapture() {
    std::lock_guard<std::mutex> lock(g_cbMutex);
    g_activeModule = nullptr;
    g_progressHandler.clear();
}

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
                    Symbols::Variables::Type::BOOLEAN, "Load a model (see README for all keys)");
    REGISTER_METHOD(this->name(), "isLoaded", {},
                    [this](FunctionArguments & args) { return this->isLoaded(args); },
                    Symbols::Variables::Type::BOOLEAN, "Whether a model is loaded");
    REGISTER_METHOD(this->name(), "unload", {},
                    [this](FunctionArguments & args) { return this->unload(args); },
                    Symbols::Variables::Type::NULL_TYPE, "Free the loaded model");
    REGISTER_METHOD(this->name(), "txt2img", opts,
                    [this](FunctionArguments & args) { return this->txt2img(args); },
                    Symbols::Variables::Type::OBJECT, "Text-to-image; returns array of output paths");
    REGISTER_METHOD(this->name(), "img2img", opts,
                    [this](FunctionArguments & args) { return this->img2img(args); },
                    Symbols::Variables::Type::OBJECT, "Image-to-image; returns array of output paths");
    REGISTER_METHOD(this->name(), "video", opts,
                    [this](FunctionArguments & args) { return this->video(args); },
                    Symbols::Variables::Type::OBJECT,
                    "Generate video frames (needs a video-capable model); returns array of frame paths");
    REGISTER_METHOD(this->name(), "upscale", opts,
                    [this](FunctionArguments & args) { return this->upscale(args); },
                    Symbols::Variables::Type::STRING, "ESRGAN upscale; returns output path");
    REGISTER_METHOD(this->name(), "getLog", {},
                    [this](FunctionArguments & args) { return this->getLog(args); },
                    Symbols::Variables::Type::STRING, "Captured log text since the last clearLog()");
    REGISTER_METHOD(this->name(), "getProgress", {},
                    [this](FunctionArguments & args) { return this->getProgress(args); },
                    Symbols::Variables::Type::OBJECT, "Array of { step, total, time } sampling ticks");
    REGISTER_METHOD(this->name(), "clearLog", {},
                    [this](FunctionArguments & args) { return this->clearLog(args); },
                    Symbols::Variables::Type::NULL_TYPE, "Clear captured log and progress");
}

Symbols::ValuePtr SDModule::construct(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("StableDiffusion::__construct must be called on a StableDiffusion instance");
    }
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
    const long   id = Symbols::ValuePtr::instanceId(args[0]);
    const auto & o  = optionsOf(args, "loadModel");

    if (auto it = contexts_.find(id); it != contexts_.end() && it->second) {
        free_sd_ctx(it->second);
        contexts_.erase(it);
    }

    sd_ctx_params_t p;
    sd_ctx_params_init(&p);

    // const char* fields point into these; a std::list keeps element addresses stable.
    std::list<std::string> keep;
    auto S = [&](const char ** field, const char * key) {
        std::string v = optStr(o, key);
        if (!v.empty()) {
            keep.push_back(std::move(v));
            *field = keep.back().c_str();
        }
    };

    S(&p.model_path, "model_path");
    S(&p.clip_l_path, "clip_l_path");
    S(&p.clip_g_path, "clip_g_path");
    S(&p.clip_vision_path, "clip_vision_path");
    S(&p.t5xxl_path, "t5xxl_path");
    S(&p.llm_path, "llm_path");
    S(&p.diffusion_model_path, "diffusion_model_path");
    S(&p.high_noise_diffusion_model_path, "high_noise_diffusion_model_path");
    S(&p.vae_path, "vae_path");
    S(&p.taesd_path, "taesd_path");
    S(&p.control_net_path, "control_net_path");
    S(&p.motion_module_path, "motion_module_path");
    S(&p.photo_maker_path, "photo_maker_path");
    S(&p.pulid_weights_path, "pulid_weights_path");
    S(&p.embeddings_connectors_path, "embeddings_connectors_path");
    S(&p.tensor_type_rules, "tensor_type_rules");
    S(&p.max_vram, "max_vram");
    S(&p.backend, "backend");
    S(&p.params_backend, "params_backend");
    S(&p.split_mode, "split_mode");
    S(&p.rpc_servers, "rpc_servers");
    S(&p.model_args, "model_args");

    if (p.model_path == nullptr && p.diffusion_model_path == nullptr) {
        throw std::runtime_error("StableDiffusion::loadModel: 'model_path' or 'diffusion_model_path' is required");
    }

    const std::string wtype       = optStr(o, "wtype");
    const std::string rng         = optStr(o, "rng_type");
    const std::string sampler_rng = optStr(o, "sampler_rng_type");
    const std::string prediction  = optStr(o, "prediction");
    const std::string lora_mode   = optStr(o, "lora_apply_mode");
    if (!wtype.empty())       p.wtype            = str_to_sd_type(wtype.c_str());
    if (!rng.empty())         p.rng_type         = str_to_rng_type(rng.c_str());
    if (!sampler_rng.empty()) p.sampler_rng_type = str_to_rng_type(sampler_rng.c_str());
    if (!prediction.empty())  p.prediction       = str_to_prediction(prediction.c_str());
    if (!lora_mode.empty())   p.lora_apply_mode  = str_to_lora_apply_mode(lora_mode.c_str());

    p.n_threads                  = static_cast<int>(optInt(o, "n_threads", p.n_threads));
    p.enable_mmap                = optBool(o, "enable_mmap", p.enable_mmap);
    p.flash_attn                 = optBool(o, "flash_attn", p.flash_attn);
    p.diffusion_flash_attn       = optBool(o, "diffusion_flash_attn", p.diffusion_flash_attn);
    p.tae_preview_only           = optBool(o, "tae_preview_only", p.tae_preview_only);
    p.diffusion_conv_direct      = optBool(o, "diffusion_conv_direct", p.diffusion_conv_direct);
    p.vae_conv_direct            = optBool(o, "vae_conv_direct", p.vae_conv_direct);
    p.force_sdxl_vae_conv_scale  = optBool(o, "force_sdxl_vae_conv_scale", p.force_sdxl_vae_conv_scale);
    p.stream_layers              = optBool(o, "stream_layers", p.stream_layers);
    p.eager_load                 = optBool(o, "eager_load", p.eager_load);
    p.auto_fit                   = optBool(o, "auto_fit", p.auto_fit);

    g_quiet = optBool(o, "quiet", false);

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

Symbols::ValuePtr SDModule::txt2img(FunctionArguments & args) { return generate(args, "txt2img", false); }
Symbols::ValuePtr SDModule::img2img(FunctionArguments & args) { return generate(args, "img2img", true); }

Symbols::ValuePtr SDModule::generate(FunctionArguments & args, const char * method, bool isImg2Img) {
    const long   id  = Symbols::ValuePtr::instanceId(args[0]);
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

    const std::string negative     = optStr(o, "negative_prompt");
    const std::string sampler      = optStr(o, "sampler");
    const std::string scheduler    = optStr(o, "scheduler");
    const std::string extra_sample = optStr(o, "extra_sample_args");

    sd_img_gen_params_t p;
    sd_img_gen_params_init(&p);
    p.prompt          = prompt.c_str();
    p.negative_prompt = negative.c_str();
    p.width           = static_cast<int>(optInt(o, "width", 512));
    p.height          = static_cast<int>(optInt(o, "height", 512));
    p.clip_skip       = static_cast<int>(optInt(o, "clip_skip", p.clip_skip));
    p.seed            = optInt(o, "seed", -1);
    p.batch_count     = static_cast<int>(optInt(o, "batch_count", 1));
    p.circular_x      = optBool(o, "circular_x", p.circular_x);
    p.circular_y      = optBool(o, "circular_y", p.circular_y);

    p.sample_params.sample_steps     = static_cast<int>(optInt(o, "steps", 20));
    p.sample_params.eta              = static_cast<float>(optNum(o, "eta", p.sample_params.eta));
    p.sample_params.shifted_timestep = static_cast<int>(optInt(o, "shifted_timestep", p.sample_params.shifted_timestep));
    p.sample_params.flow_shift       = static_cast<float>(optNum(o, "flow_shift", p.sample_params.flow_shift));
    p.sample_params.guidance.txt_cfg = static_cast<float>(optNum(o, "cfg_scale", 7.0));
    p.sample_params.guidance.img_cfg = static_cast<float>(optNum(o, "img_cfg", p.sample_params.guidance.img_cfg));
    p.sample_params.guidance.distilled_guidance =
        static_cast<float>(optNum(o, "distilled_guidance", p.sample_params.guidance.distilled_guidance));
    if (!sampler.empty())      p.sample_params.sample_method     = str_to_sample_method(sampler.c_str());
    if (!scheduler.empty())    p.sample_params.scheduler         = str_to_scheduler(scheduler.c_str());
    if (!extra_sample.empty()) p.sample_params.extra_sample_args = extra_sample.c_str();

    // LoRAs (multiple). Kept alive until generate_image returns.
    std::list<std::string> loraKeep;
    std::vector<sd_lora_t>  loras;
    readLoras(o, loraKeep, loras);
    if (!loras.empty()) {
        p.loras      = loras.data();
        p.lora_count = static_cast<uint32_t>(loras.size());
    }
    readTiling(o, "vae_tiling", p.vae_tiling_params, loraKeep);

    sd_image_t control{};
    const std::string control_path = optStr(o, "control_image");
    if (!control_path.empty()) {
        control            = loadImage(control_path);
        p.control_image    = control;
        p.control_strength = static_cast<float>(optNum(o, "control_strength", 0.9));
    }

    // Reference/edit images (e.g. Mage `-r` edit). Applies to txt2img and img2img alike,
    // so it is read unconditionally rather than gated on isImg2Img.
    std::vector<sd_image_t> refs;
    readRefImages(o, refs, loraKeep, &p.ref_image_args);
    if (!refs.empty()) {
        p.ref_images       = refs.data();
        p.ref_images_count = static_cast<int>(refs.size());
    }

    sd_image_t init{};
    sd_image_t mask{};
    if (isImg2Img) {
        const std::string init_path = optStr(o, "init_image");
        if (init_path.empty()) {
            throw std::runtime_error("StableDiffusion::img2img: 'init_image' path is required");
        }
        init         = loadImage(init_path);
        p.init_image = init;
        p.strength   = static_cast<float>(optNum(o, "strength", 0.75));
        const std::string mask_path = optStr(o, "mask_image");
        if (!mask_path.empty()) {
            mask         = loadImage(mask_path);
            p.mask_image = mask;
        }
    }

    const std::string progressHandler = optStr(o, "progress");

    sd_image_t * out     = nullptr;
    int          num_out = 0;
    beginCapture(id, progressHandler);
    const bool ok = generate_image(ctx, &p, &out, &num_out);
    endCapture();

    if (init.data)    { stbi_image_free(init.data); }
    if (mask.data)    { stbi_image_free(mask.data); }
    if (control.data) { stbi_image_free(control.data); }
    for (auto & im : refs) {
        if (im.data) { stbi_image_free(im.data); }
    }

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

Symbols::ValuePtr SDModule::video(FunctionArguments & args) {
    const long   id  = Symbols::ValuePtr::instanceId(args[0]);
    sd_ctx_t *   ctx = contextFor(args, "video");
    const auto & o   = optionsOf(args, "video");

    if (!sd_ctx_supports_video_generation(ctx)) {
        throw std::runtime_error("StableDiffusion::video: the loaded model does not support video generation "
                                 "(load a video model, e.g. WAN / SVD / AnimateDiff)");
    }

    const std::string prompt = optStr(o, "prompt");
    const std::string output = optStr(o, "output");
    if (prompt.empty()) {
        throw std::runtime_error("StableDiffusion::video: 'prompt' is required");
    }
    if (output.empty()) {
        throw std::runtime_error("StableDiffusion::video: 'output' path is required");
    }
    const std::string negative  = optStr(o, "negative_prompt");
    const std::string sampler   = optStr(o, "sampler");
    const std::string scheduler = optStr(o, "scheduler");

    sd_vid_gen_params_t p;
    sd_vid_gen_params_init(&p);
    p.prompt          = prompt.c_str();
    p.negative_prompt = negative.c_str();
    p.width           = static_cast<int>(optInt(o, "width", 512));
    p.height          = static_cast<int>(optInt(o, "height", 512));
    p.clip_skip       = static_cast<int>(optInt(o, "clip_skip", p.clip_skip));
    p.seed            = optInt(o, "seed", -1);
    p.video_frames    = static_cast<int>(optInt(o, "video_frames", 16));
    p.fps             = static_cast<int>(optInt(o, "fps", 8));
    p.strength        = static_cast<float>(optNum(o, "strength", p.strength));
    p.moe_boundary    = static_cast<float>(optNum(o, "moe_boundary", p.moe_boundary));
    p.vace_strength   = static_cast<float>(optNum(o, "vace_strength", p.vace_strength));
    p.circular_x      = optBool(o, "circular_x", p.circular_x);
    p.circular_y      = optBool(o, "circular_y", p.circular_y);

    p.sample_params.sample_steps     = static_cast<int>(optInt(o, "steps", 20));
    p.sample_params.guidance.txt_cfg = static_cast<float>(optNum(o, "cfg_scale", 7.0));
    if (!sampler.empty())   p.sample_params.sample_method = str_to_sample_method(sampler.c_str());
    if (!scheduler.empty()) p.sample_params.scheduler     = str_to_scheduler(scheduler.c_str());

    std::list<std::string> loraKeep;
    std::vector<sd_lora_t>  loras;
    readLoras(o, loraKeep, loras);
    if (!loras.empty()) {
        p.loras      = loras.data();
        p.lora_count = static_cast<uint32_t>(loras.size());
    }
    readTiling(o, "vae_tiling", p.vae_tiling_params, loraKeep);

    // Optional first/last frame conditioning.
    sd_image_t init{};
    sd_image_t end{};
    const std::string init_path = optStr(o, "init_image");
    const std::string end_path  = optStr(o, "end_image");
    if (!init_path.empty()) { init = loadImage(init_path); p.init_image = init; }
    if (!end_path.empty())  { end  = loadImage(end_path);  p.end_image  = end; }

    const std::string progressHandler = optStr(o, "progress");

    sd_image_t * frames    = nullptr;
    int          numFrames = 0;
    sd_audio_t * audio     = nullptr;
    beginCapture(id, progressHandler);
    const bool ok = generate_video(ctx, &p, &frames, &numFrames, &audio);
    endCapture();

    if (init.data) { stbi_image_free(init.data); }
    if (end.data)  { stbi_image_free(end.data); }

    if (!ok || !frames || numFrames <= 0) {
        if (frames) { free_sd_images(frames, numFrames); }
        if (audio)  { free_sd_audio(audio); }
        throw std::runtime_error("StableDiffusion::video: generation failed");
    }

    // Frames are always indexed: "vid.png" -> vid_0.png, vid_1.png, ...
    Symbols::ObjectMap paths;
    try {
        for (int i = 0; i < numFrames; ++i) {
            const auto        dot  = output.rfind('.');
            const std::string path = (dot == std::string::npos)
                                         ? output + "_" + std::to_string(i)
                                         : output.substr(0, dot) + "_" + std::to_string(i) + output.substr(dot);
            writePng(frames[i], path);
            paths[std::to_string(i)] = Symbols::ValuePtr(path);
        }
    } catch (...) {
        free_sd_images(frames, numFrames);
        if (audio) { free_sd_audio(audio); }
        throw;
    }
    free_sd_images(frames, numFrames);
    if (audio) { free_sd_audio(audio); }
    return Symbols::ValuePtr(paths);
}

Symbols::ValuePtr SDModule::upscale(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("StableDiffusion::upscale must be called on a StableDiffusion instance");
    }
    const long   id = Symbols::ValuePtr::instanceId(args[0]);
    const auto & o  = optionsOf(args, "upscale");

    const std::string esrgan = optStr(o, "esrgan_path");
    const std::string input  = optStr(o, "input");
    const std::string output = optStr(o, "output");
    if (esrgan.empty() || input.empty() || output.empty()) {
        throw std::runtime_error("StableDiffusion::upscale requires 'esrgan_path', 'input' and 'output'");
    }
    const int factor    = static_cast<int>(optInt(o, "scale", 4));
    const int n_threads = static_cast<int>(optInt(o, "n_threads", sd_get_num_physical_cores()));
    const int tile      = static_cast<int>(optInt(o, "tile_size", 0));

    upscaler_ctx_t * up = new_upscaler_ctx(esrgan.c_str(), false, n_threads, tile, nullptr, nullptr);
    if (!up) {
        throw std::runtime_error("StableDiffusion::upscale: failed to load ESRGAN model '" + esrgan + "'");
    }

    sd_image_t   in      = loadImage(input);
    sd_image_t * out     = nullptr;
    int          num_out = 0;
    beginCapture(id, optStr(o, "progress"));
    const bool ok = ::upscale(up, in, static_cast<uint32_t>(factor), &out, &num_out);
    endCapture();
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

Symbols::ValuePtr SDModule::getLog(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = logs_.find(id);
    return Symbols::ValuePtr(it == logs_.end() ? std::string{} : it->second);
}

Symbols::ValuePtr SDModule::getProgress(FunctionArguments & args) {
    const long         id = Symbols::ValuePtr::instanceId(args[0]);
    Symbols::ObjectMap out;
    auto               it = progress_.find(id);
    if (it != progress_.end()) {
        int i = 0;
        for (const auto & r : it->second) {
            Symbols::ObjectMap rec;
            rec["step"]              = Symbols::ValuePtr(r.step);
            rec["total"]             = Symbols::ValuePtr(r.total);
            rec["time"]              = Symbols::ValuePtr(static_cast<double>(r.time));
            out[std::to_string(i++)] = Symbols::ValuePtr(rec);
        }
    }
    return Symbols::ValuePtr(out);
}

Symbols::ValuePtr SDModule::clearLog(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    logs_.erase(id);
    progress_.erase(id);
    return Symbols::ValuePtr::null();
}

}  // namespace Modules
