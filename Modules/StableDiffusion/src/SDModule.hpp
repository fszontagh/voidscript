#ifndef SD_MODULE_HPP
#define SD_MODULE_HPP

#include <stable-diffusion.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief VoidScript class "StableDiffusion" wrapping stable-diffusion.cpp.
 *
 * See Modules/StableDiffusion/README.md for the full option list. In brief:
 *
 *   StableDiffusion $sd = new StableDiffusion();
 *   $sd->loadModel({ string model_path: "...", ... });
 *   auto $paths = $sd->txt2img({ string prompt: "...", string output: "/tmp/o.png",
 *                               int width: 512, int height: 512, int steps: 20,
 *                               double cfg_scale: 7.0, int seed: 42 });
 *   string $log = $sd->getLog();          // captured log text for this instance
 *   auto   $prog = $sd->getProgress();     // [ { step, total, time }, ... ]
 *
 * Each instance holds its own sd_ctx_t*, keyed by the framework instance id (never by
 * args[0].toString(), which collides every instance).
 */
class SDModule : public BaseModule {
  public:
    SDModule() {
        setModuleName("StableDiffusion");
        setDescription("Text-to-image, image-to-image and upscaling via stable-diffusion.cpp");
    }

    void registerFunctions() override;

    // One progress tick reported by sd.cpp during sampling.
    struct ProgressRec {
        int   step;
        int   total;
        float time;
    };

    // Called by the C trampolines (module-global) for the currently active instance.
    void appendLog(const char * text);
    void appendProgress(int step, int steps, float time);

  private:
    std::unordered_map<long, sd_ctx_t *>                contexts_;
    std::unordered_map<long, std::string>               logs_;
    std::unordered_map<long, std::vector<ProgressRec>>  progress_;

    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr loadModel(FunctionArguments & args);
    Symbols::ValuePtr isLoaded(FunctionArguments & args);
    Symbols::ValuePtr unload(FunctionArguments & args);
    Symbols::ValuePtr txt2img(FunctionArguments & args);
    Symbols::ValuePtr img2img(FunctionArguments & args);
    Symbols::ValuePtr video(FunctionArguments & args);
    Symbols::ValuePtr upscale(FunctionArguments & args);
    Symbols::ValuePtr getLog(FunctionArguments & args);
    Symbols::ValuePtr getProgress(FunctionArguments & args);
    Symbols::ValuePtr clearLog(FunctionArguments & args);

    Symbols::ValuePtr generate(FunctionArguments & args, const char * method, bool isImg2Img);
    sd_ctx_t *        contextFor(FunctionArguments & args, const char * method);

    // Route sd.cpp's global log/progress callbacks to this instance for the duration of
    // a generation, then unset. Generation is synchronous, so only one is ever active.
    void beginCapture(long id);
    void endCapture();
};

}  // namespace Modules

#endif  // SD_MODULE_HPP
