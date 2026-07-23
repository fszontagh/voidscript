#include "PixelArtModule.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>

#include "config.hpp"    // PixelateConfig (from PIXELART_ROOT/src)
#include "pixelate.hpp"  // pixelate()

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace fs = std::filesystem;

namespace Modules {

namespace {

// --- option readers over the options ObjectMap -------------------------------------

const Symbols::ObjectMap & optionsOf(FunctionArguments & args) {
    if (args.size() < 2 ||
        (args[1] != Symbols::Variables::Type::OBJECT && args[1] != Symbols::Variables::Type::CLASS)) {
        throw std::runtime_error("PixelArt::convert expects an options object");
    }
    return args[1]->get<Symbols::ObjectMap>();
}

std::string optStr(const Symbols::ObjectMap & o, const char * key, const std::string & def = "") {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error(std::string("PixelArt: option '") + key + "' must be a string");
    }
    return it->second->get<std::string>();
}

long optInt(const Symbols::ObjectMap & o, const char * key, long def) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error(std::string("PixelArt: option '") + key + "' must be an integer");
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
            throw std::runtime_error(std::string("PixelArt: option '") + key + "' must be a number");
    }
}

bool optBool(const Symbols::ObjectMap & o, const char * key, bool def) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return def;
    }
    if (it->second->getType() != Symbols::Variables::Type::BOOLEAN) {
        throw std::runtime_error(std::string("PixelArt: option '") + key + "' must be a boolean");
    }
    return it->second->get<bool>();
}

// Returns a pointer to a nested options object under `key`, or nullptr if absent. Lets
// advanced callers override the mesh/colors tuning knobs without cluttering the common
// path.
const Symbols::ObjectMap * optObj(const Symbols::ObjectMap & o, const char * key) {
    auto it = o.find(key);
    if (it == o.end() || it->second->is_null()) {
        return nullptr;
    }
    if (it->second->getType() != Symbols::Variables::Type::OBJECT &&
        it->second->getType() != Symbols::Variables::Type::CLASS) {
        throw std::runtime_error(std::string("PixelArt: option '") + key + "' must be an object");
    }
    return &it->second->get<Symbols::ObjectMap>();
}

// Mirror sdpixel2realpixelart's main.cpp: an output with a real extension is a file path,
// otherwise it is a directory and the name is derived from the input stem.
std::string resolveOutput(const std::string & out_arg, const std::string & input_path) {
    fs::path out(out_arg);
    if (out.has_extension() && out.extension() != ".") {
        return out.string();
    }
    fs::path stem = fs::path(input_path).stem();
    return (out / (stem.string() + "_result.png")).string();
}

void applyMeshConfig(const Symbols::ObjectMap & m, MeshConfig & mesh) {
    mesh.crop_border_pixels    = static_cast<int>(optInt(m, "crop_border_pixels", mesh.crop_border_pixels));
    mesh.canny_low             = optNum(m, "canny_low", mesh.canny_low);
    mesh.canny_high            = optNum(m, "canny_high", mesh.canny_high);
    mesh.closure_kernel_size   = static_cast<int>(optInt(m, "closure_kernel_size", mesh.closure_kernel_size));
    mesh.cluster_threshold     = static_cast<int>(optInt(m, "cluster_threshold", mesh.cluster_threshold));
    mesh.angle_threshold_deg   = optNum(m, "angle_threshold_deg", mesh.angle_threshold_deg);
    mesh.trim_outlier_fraction = optNum(m, "trim_outlier_fraction", mesh.trim_outlier_fraction);
    if (const Symbols::ObjectMap * h = optObj(m, "hough")) {
        mesh.hough.rho          = optNum(*h, "rho", mesh.hough.rho);
        mesh.hough.theta_deg    = optNum(*h, "theta_deg", mesh.hough.theta_deg);
        mesh.hough.threshold    = static_cast<int>(optInt(*h, "threshold", mesh.hough.threshold));
        mesh.hough.min_line_len = static_cast<int>(optInt(*h, "min_line_len", mesh.hough.min_line_len));
        mesh.hough.max_line_gap = static_cast<int>(optInt(*h, "max_line_gap", mesh.hough.max_line_gap));
    }
}

void applyColorConfig(const Symbols::ObjectMap & c, ColorConfig & colors) {
    colors.alpha_threshold                = static_cast<int>(optInt(c, "alpha_threshold", colors.alpha_threshold));
    colors.transparency_majority_fraction = optNum(c, "transparency_majority_fraction",
                                                    colors.transparency_majority_fraction);
    colors.bin_size                       = static_cast<int>(optInt(c, "bin_size", colors.bin_size));
    colors.top_colors_limit               = static_cast<int>(optInt(c, "top_colors_limit", colors.top_colors_limit));
    colors.thumbnail_w                    = static_cast<int>(optInt(c, "thumbnail_w", colors.thumbnail_w));
    colors.thumbnail_h                    = static_cast<int>(optInt(c, "thumbnail_h", colors.thumbnail_h));
}

}  // namespace

void PixelArtModule::registerFunctions() {
    REGISTER_CLASS(this->name());

    REGISTER_METHOD(this->name(), "__construct", {},
                    [this](FunctionArguments & args) { return this->construct(args); },
                    Symbols::Variables::Type::CLASS, "Create a PixelArt instance");

    std::vector<Symbols::FunctionParameterInfo> opts = {
        { "options", Symbols::Variables::Type::OBJECT, "Options object (see README for all keys)" }
    };
    REGISTER_METHOD(this->name(), "convert", opts,
                    [this](FunctionArguments & args) { return this->convert(args); },
                    Symbols::Variables::Type::STRING,
                    "Fix a fake-pixel-art image; returns the written output path");
}

Symbols::ValuePtr PixelArtModule::construct(FunctionArguments & args) {
    // Stateless transform: nothing to stamp onto the instance. `new` returns args[0].
    return args[0];
}

Symbols::ValuePtr PixelArtModule::convert(FunctionArguments & args) {
    const Symbols::ObjectMap & o = optionsOf(args);

    const std::string input = optStr(o, "input");
    if (input.empty()) {
        throw std::runtime_error("PixelArt::convert: 'input' path is required");
    }
    const std::string output = optStr(o, "output", ".");

    PixelateConfig cfg;
    cfg.num_colors             = static_cast<int>(optInt(o, "colors", cfg.num_colors));
    cfg.scale_result           = static_cast<int>(optInt(o, "scale_result", cfg.scale_result));
    cfg.initial_upscale_factor = static_cast<int>(optInt(o, "initial_upscale", cfg.initial_upscale_factor));
    cfg.pixel_width            = static_cast<int>(optInt(o, "pixel_width", cfg.pixel_width));
    cfg.transparent_background = optBool(o, "transparent", cfg.transparent_background);
    if (const Symbols::ObjectMap * m = optObj(o, "mesh")) {
        applyMeshConfig(*m, cfg.mesh);
    }
    if (const Symbols::ObjectMap * c = optObj(o, "colors_config")) {
        applyColorConfig(*c, cfg.colors);
    }

    const std::string debug_dir = optStr(o, "debug_dir");

    cv::Mat img = cv::imread(input, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::runtime_error("PixelArt::convert: cannot read image '" + input + "'");
    }
    // Ensure BGRA, matching the CLI's preprocessing.
    if (img.channels() == 1) {
        cv::cvtColor(img, img, cv::COLOR_GRAY2BGRA);
    } else if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);
    }

    if (!debug_dir.empty()) {
        fs::create_directories(debug_dir);
    }

    cv::Mat result = pixelate(img, cfg, debug_dir);

    const std::string out_path = resolveOutput(output, input);
    const fs::path     parent  = fs::path(out_path).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
    if (!cv::imwrite(out_path, result)) {
        throw std::runtime_error("PixelArt::convert: could not write output to '" + out_path + "'");
    }
    return Symbols::ValuePtr(out_path);
}

}  // namespace Modules
