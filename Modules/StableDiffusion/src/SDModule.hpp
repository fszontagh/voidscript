#ifndef SD_MODULE_HPP
#define SD_MODULE_HPP

#include <stable-diffusion.h>

#include <string>
#include <unordered_map>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief VoidScript class "StableDiffusion" wrapping stable-diffusion.cpp.
 *
 *   StableDiffusion $sd = new StableDiffusion();
 *   $sd->loadModel({ string model_path: "...", string vae_path: "...", ... });
 *   $sd->txt2img({ string prompt: "a cat", string output: "/tmp/out.png",
 *                  int width: 512, int height: 512, int steps: 20,
 *                  double cfg_scale: 7.0, int seed: 42, string sampler: "euler_a" });
 *   $sd->img2img({ ... string init_image: "in.png", double strength: 0.6, ... });
 *   $sd->upscale({ string esrgan_path: "...", string input: "a.png",
 *                  string output: "a4x.png", int scale: 4 });
 *
 * The sd_ctx_t* is held per instance, keyed by the object's framework instance id -
 * NOT by args[0].toString(), which serialises object contents and collides every
 * instance (the bug fixed across the other modules).
 */
class SDModule : public BaseModule {
  public:
    SDModule() {
        setModuleName("StableDiffusion");
        setDescription("Text-to-image, image-to-image and upscaling via stable-diffusion.cpp");
    }

    void registerFunctions() override;

  private:
    // Per-instance loaded context, keyed by Symbols::ValuePtr::instanceId(self).
    std::unordered_map<long, sd_ctx_t *> contexts_;

    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr loadModel(FunctionArguments & args);
    Symbols::ValuePtr isLoaded(FunctionArguments & args);
    Symbols::ValuePtr unload(FunctionArguments & args);
    Symbols::ValuePtr txt2img(FunctionArguments & args);
    Symbols::ValuePtr img2img(FunctionArguments & args);
    Symbols::ValuePtr upscale(FunctionArguments & args);

    // Shared generation path for txt2img / img2img (img2img sets init_image + strength).
    Symbols::ValuePtr generate(FunctionArguments & args, const char * method, bool isImg2Img);

    // Returns the context for this instance, or throws if loadModel was not called.
    sd_ctx_t * contextFor(FunctionArguments & args, const char * method);
};

}  // namespace Modules

#endif  // SD_MODULE_HPP
