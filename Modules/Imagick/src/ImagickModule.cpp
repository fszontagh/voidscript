#include "ImagickModule.hpp"

#include <filesystem>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

// Bring MagickCore's Quantum type into scope so the QuantumRange macro (which casts to a
// bare `Quantum`) expands under the Magick++ C++ namespacing.
using MagickCore::Quantum;

void Modules::ImagickModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> params = {
        { "filename", Symbols::Variables::Type::STRING, "The image file to manipulate" },
    };

    REGISTER_CLASS(this->name());

    // Register constructor
    REGISTER_METHOD(
        this->name(), "__construct", {}, [this](const FunctionArguments & args) { return this->construct(args); },
        Symbols::Variables::Type::CLASS, "Constructor for Imagick class");

    REGISTER_METHOD(
        this->name(), "read", params, [this](const FunctionArguments & args) { return this->read(args); },
        Symbols::Variables::Type::CLASS, "Read an image file");

    params = { { "filename", Symbols::Variables::Type::STRING, "Output file" },
               { "quality", Symbols::Variables::Type::INTEGER, "JPEG/compression quality 1-100 (optional)", true } };
    REGISTER_METHOD(
        this->name(), "write", params, [this](const FunctionArguments & args) { return this->write(args); },
        Symbols::Variables::Type::NULL_TYPE, "Save the image (optional quality for JPEG/WebP/etc.)");

    params = {
        { "width",   Symbols::Variables::Type::INTEGER, "The width of the crop"                                    },
        { "height",  Symbols::Variables::Type::INTEGER, "The height of the crop"                                   },
        { "xOffset", Symbols::Variables::Type::INTEGER, "The X coordinate of the cropped region's top left corner" },
        { "yOffset", Symbols::Variables::Type::INTEGER, "The Y coordinate of the cropped region's top left corner" },
    };

    REGISTER_METHOD(
        this->name(), "crop", params, [this](const FunctionArguments & args) { return this->crop(args); },
        Symbols::Variables::Type::NULL_TYPE, "Extracts a region of the image");

    params = {
        { "width",   Symbols::Variables::Type::INTEGER, "New width, or a geometry string like \"512x768\" / \"512x768!\"" },
        { "height",  Symbols::Variables::Type::INTEGER, "New height (omit only for the geometry-string form)", true },
        { "bestFit", Symbols::Variables::Type::BOOLEAN, "Preserve aspect ratio (fit inside); default false = exact", true }
    };

    REGISTER_METHOD(
        this->name(), "resize", params, [this](const FunctionArguments & args) { return this->resize(args); },
        Symbols::Variables::Type::NULL_TYPE,
        "Resize to exactly width x height (pass bestFit=true to preserve aspect ratio)");

    params = {
        { "mode", Symbols::Variables::Type::STRING, "Colorspace: RGB, CMYK or GRAY" },
    };
    REGISTER_METHOD(
        this->name(), "mode", params, [this](const FunctionArguments & args) { return this->mode(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set the image colorspace (RGB, CMYK, GRAY)");

    params = {
        { "radius", Symbols::Variables::Type::DOUBLE, "The blur radius" },
        { "sigma",  Symbols::Variables::Type::DOUBLE, "The standard deviation of the blur" },
    };

    REGISTER_METHOD(
        this->name(), "blur", params, [this](const FunctionArguments & args) { return this->blur(args); },
        Symbols::Variables::Type::NULL_TYPE, "Blur an image");

    params = {
        { "degrees", Symbols::Variables::Type::DOUBLE, "The angle in degrees to rotate the image" },
    };

    REGISTER_METHOD(
        this->name(), "rotate", params, [this](const FunctionArguments & args) { return this->rotate(args); },
        Symbols::Variables::Type::NULL_TYPE, "Rotate image");

    params = {
        { "direction", Symbols::Variables::Type::STRING, "The direction to flip the image (horizontal or vertical)" },
    };

    REGISTER_METHOD(
        this->name(), "flip", params, [this](const FunctionArguments & args) { return this->flip(args); },
        Symbols::Variables::Type::NULL_TYPE, "Flip image");
    REGISTER_METHOD(
        this->name(), "getWidth", {}, [this](const FunctionArguments & args) { return this->getWidth(args); },
        Symbols::Variables::Type::INTEGER, "Get the width of the image");
    REGISTER_METHOD(
        this->name(), "getHeight", {}, [this](const FunctionArguments & args) { return this->getHeight(args); },
        Symbols::Variables::Type::INTEGER, "Get the height of the image");
    params = { { "x", Symbols::Variables::Type::INTEGER, "X coordinate" },
               { "y", Symbols::Variables::Type::INTEGER, "Y coordinate" } };
    REGISTER_METHOD(
        this->name(), "getPixel", params, [this](const FunctionArguments & args) { return this->getPixel(args); },
        Symbols::Variables::Type::OBJECT, "Read a pixel as { red, green, blue, alpha }, each 0-255");
    params = { { "x", Symbols::Variables::Type::INTEGER, "X coordinate" },
               { "y", Symbols::Variables::Type::INTEGER, "Y coordinate" },
               { "red", Symbols::Variables::Type::INTEGER, "Red 0-255" },
               { "green", Symbols::Variables::Type::INTEGER, "Green 0-255" },
               { "blue", Symbols::Variables::Type::INTEGER, "Blue 0-255" },
               { "alpha", Symbols::Variables::Type::INTEGER, "Alpha 0-255", true } };
    REGISTER_METHOD(
        this->name(), "setPixel", params, [this](const FunctionArguments & args) { return this->setPixel(args); },
        Symbols::Variables::Type::NULL_TYPE, "Write a pixel from 0-255 channel values");
    params = { { "source", Symbols::Variables::Type::CLASS,   "Imagick image to overlay" },
               { "x",      Symbols::Variables::Type::INTEGER, "X offset of the overlay" },
               { "y",      Symbols::Variables::Type::INTEGER, "Y offset of the overlay" } };
    REGISTER_METHOD(
        this->name(), "composite", params, [this](const FunctionArguments & args) { return this->composite(args); },
        Symbols::Variables::Type::NULL_TYPE, "Overlay another image at (x, y) using the Over operator");

    params = { { "width",    Symbols::Variables::Type::INTEGER, "Canvas width in pixels" },
               { "height",   Symbols::Variables::Type::INTEGER, "Canvas height in pixels" },
               { "colorHex", Symbols::Variables::Type::STRING,  "Fill color, e.g. \"#000000\"" } };
    REGISTER_METHOD(
        this->name(), "newImage", params, [this](const FunctionArguments & args) { return this->newImage(args); },
        Symbols::Variables::Type::CLASS, "Create a new solid-color canvas of the given size");

    params = { { "width",      Symbols::Variables::Type::INTEGER, "Canvas width in pixels" },
               { "height",     Symbols::Variables::Type::INTEGER, "Canvas height in pixels" },
               { "startColor", Symbols::Variables::Type::STRING,  "Top color, e.g. \"#ffffff\"" },
               { "endColor",   Symbols::Variables::Type::STRING,  "Bottom color, e.g. \"#000000\"" } };
    REGISTER_METHOD(
        this->name(), "gradient", params, [this](const FunctionArguments & args) { return this->gradient(args); },
        Symbols::Variables::Type::CLASS, "Create a native linear (top-to-bottom) gradient canvas");

    params = { { "width",       Symbols::Variables::Type::INTEGER, "Canvas width in pixels" },
               { "height",      Symbols::Variables::Type::INTEGER, "Canvas height in pixels" },
               { "innerColor",  Symbols::Variables::Type::STRING,  "Center color, e.g. \"#ffffff\"" },
               { "outerColor",  Symbols::Variables::Type::STRING,  "Edge color, e.g. \"#000000\"" } };
    REGISTER_METHOD(
        this->name(), "radialGradient", params,
        [this](const FunctionArguments & args) { return this->radialGradient(args); },
        Symbols::Variables::Type::CLASS,
        "Create a native radial (center-to-edge) gradient canvas - e.g. a vignette mask");

    params = { { "width",    Symbols::Variables::Type::INTEGER, "Target width" },
               { "height",   Symbols::Variables::Type::INTEGER, "Target height" },
               { "xOffset",  Symbols::Variables::Type::INTEGER, "X offset of the current image within the new extent" },
               { "yOffset",  Symbols::Variables::Type::INTEGER, "Y offset of the current image within the new extent" },
               { "colorHex", Symbols::Variables::Type::STRING,  "Background fill for padded area (optional)", true } };
    REGISTER_METHOD(
        this->name(), "extent", params, [this](const FunctionArguments & args) { return this->extent(args); },
        Symbols::Variables::Type::NULL_TYPE, "Pad or crop the image to an exact size, filling new area with a color");

    params = { { "type",     Symbols::Variables::Type::STRING, "gaussian|uniform|impulse|laplacian|poisson|multiplicative|random" },
               { "strength", Symbols::Variables::Type::DOUBLE, "Attenuate factor; higher = MORE noise for gaussian/multiplicative" } };
    REGISTER_METHOD(
        this->name(), "addNoise", params, [this](const FunctionArguments & args) { return this->addNoise(args); },
        Symbols::Variables::Type::NULL_TYPE,
        "Add noise natively. Use 'gaussian' or 'multiplicative' for calibrated grain; "
        "'poisson' ignores strength in a usable way (its attenuate is inverse/uncalibratable in ImageMagick)");

    params = { { "op",    Symbols::Variables::Type::STRING, "multiply|add|subtract|divide|set|min|max" },
               { "value", Symbols::Variables::Type::DOUBLE, "Right-hand value, 0-1 (e.g. 0.7 to darken)" } };
    REGISTER_METHOD(
        this->name(), "evaluate", params, [this](const FunctionArguments & args) { return this->evaluate(args); },
        Symbols::Variables::Type::NULL_TYPE, "Apply an arithmetic operator to every pixel's color channels");

    params = { { "mask", Symbols::Variables::Type::CLASS, "Imagick mask image to multiply with" } };
    REGISTER_METHOD(
        this->name(), "compositeMultiply", params,
        [this](const FunctionArguments & args) { return this->compositeMultiply(args); },
        Symbols::Variables::Type::NULL_TYPE, "Multiply this image by a mask image (native, whole-image)");

    REGISTER_METHOD(
        this->name(), "stripImage", {}, [this](const FunctionArguments & args) { return this->stripImage(args); },
        Symbols::Variables::Type::NULL_TYPE, "Remove all profiles and comments (EXIF, ICC, ...) from the image");

    params = { { "source", Symbols::Variables::Type::CLASS,   "Imagick image to composite" },
               { "op",     Symbols::Variables::Type::STRING,  "over|multiply|screen|add|subtract|difference|darken|lighten|overlay" },
               { "x",      Symbols::Variables::Type::INTEGER, "X offset (default 0)", true },
               { "y",      Symbols::Variables::Type::INTEGER, "Y offset (default 0)", true } };
    REGISTER_METHOD(
        this->name(), "compositeOp", params, [this](const FunctionArguments & args) { return this->compositeOp(args); },
        Symbols::Variables::Type::NULL_TYPE,
        "Composite another image using a named operator (image-image arithmetic: add/subtract/...)");

    params = { { "method",  Symbols::Variables::Type::STRING, "barrel|perspective|srt|affine|arc|bilinear|polar|depolar" },
               { "args",    Symbols::Variables::Type::OBJECT, "Array of double coefficients for the method" },
               { "bestfit", Symbols::Variables::Type::BOOLEAN, "Resize the canvas to fit the result (default false)", true } };
    REGISTER_METHOD(
        this->name(), "distort", params, [this](const FunctionArguments & args) { return this->distort(args); },
        Symbols::Variables::Type::NULL_TYPE, "Geometric distortion (barrel/perspective/SRT/...)");

    params = { { "channel", Symbols::Variables::Type::STRING, "red|green|blue|alpha" } };
    REGISTER_METHOD(
        this->name(), "extractChannel", params,
        [this](const FunctionArguments & args) { return this->extractChannel(args); },
        Symbols::Variables::Type::NULL_TYPE,
        "Reduce this image in place to one channel as grayscale (for per-channel warps)");

    params = { { "red",   Symbols::Variables::Type::CLASS, "Grayscale image for the red channel" },
               { "green", Symbols::Variables::Type::CLASS, "Grayscale image for the green channel" },
               { "blue",  Symbols::Variables::Type::CLASS, "Grayscale image for the blue channel" } };
    REGISTER_METHOD(
        this->name(), "combineChannels", params,
        [this](const FunctionArguments & args) { return this->combineChannels(args); },
        Symbols::Variables::Type::NULL_TYPE,
        "Set this image's R/G/B from three grayscale channel images (e.g. chromatic aberration)");
}

Symbols::ValuePtr Modules::ImagickModule::construct(FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error("Imagick::__construct expects no parameters, got: " + std::to_string(args.size() - 1));
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::__construct must be called on Imagick instance");
    }

    Symbols::ObjectMap objMap = args[0];
    
    // Initialize the object properly - no image loaded yet, so no __image_id__
    // The object is already created with $class_name, just return it as-is
    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr Modules::ImagickModule::read(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Imagick::read expects (filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::read must be called on Imagick instance");
    }

    std::string filename = args[1];

    if (!std::filesystem::exists(filename)) {
        throw std::invalid_argument("File does not exists: " + filename);
    }
    
    Magick::Image image;
    try {
        image.read(filename);
        
        // Validate the image is properly loaded
        size_t width = image.columns();
        size_t height = image.rows();
        
        if (width == 0 || height == 0) {
            throw std::runtime_error("Image loaded but has invalid dimensions (" +
                                    std::to_string(width) + "x" + std::to_string(height) + ")");
        }
        
        // Additional validation: check if image is valid
        if (!image.isValid()) {
            throw std::runtime_error("Image loaded but is marked as invalid by ImageMagick");
        }
        
        int handle = next_image_id++;
        images_[handle] = image;

        // Store the handle ON the object. Keying an external map by args[0].toString()
        // collided every instance, since a fresh Imagick serialises to the same string.
        Symbols::ValuePtr self = args[0];
        self->get<Symbols::ObjectMap>()["__image_id__"] = Symbols::ValuePtr(handle);
        return self;
    } catch (const Magick::Exception& e) {
        throw std::runtime_error("Failed to read image '" + filename + "': " + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read image '" + filename + "': " + e.what());
    }
}

Symbols::ValuePtr Modules::ImagickModule::crop(Symbols::FunctionArguments & args) {
    if (args.size() != 5) {
        throw std::invalid_argument(
            "Imagick::crop missing argument: (int width, int height, int xOffset, int , int yOffset)");
    }
    Magick::Image & image = imageFor(args, "crop");

    const int width   = args[1];
    const int height  = args[2];
    const int xOffset = args[3];
    const int yOffset = args[4];

    image.crop({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::resize(Symbols::FunctionArguments & args) {
    if (args.size() < 2) {
        throw std::invalid_argument(
            "Imagick::resize missing argument: (string $sizes | int $width, int $height, int $xOffset = 0, int $yOffset "
            "= 0)");
    }
    Magick::Image & image = imageFor(args, "resize");

    if (args[1] == Symbols::Variables::Type::STRING) {
        // String form keeps full ImageMagick geometry semantics: "512x768" fits inside
        // (aspect preserved), "512x768!" forces the exact size, plus >, <, ^ etc.
        const std::string size = args[1];
        image.resize(size);
        return Symbols::ValuePtr::null();
    }

    if (args.size() < 3) {
        throw std::invalid_argument("Imagick::resize: Missing arguments");
    }

    const int width  = args[1];
    const int height = args[2];

    // The two-int form resizes to EXACTLY width x height by default (as PHP's Imagick
    // does), instead of silently shrinking to fit the source aspect ratio. Pass a
    // trailing boolean true to opt back into aspect-preserving "best fit".
    bool bestFit = false;
    if (args.size() >= 4 && args[3] == Symbols::Variables::Type::BOOLEAN) {
        bestFit = args[3]->get<bool>();
    }

    Magick::Geometry geometry(static_cast<size_t>(width), static_cast<size_t>(height));
    geometry.aspect(!bestFit);   // aspect(true) == the '!' flag: ignore the source ratio
    image.resize(geometry);
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::write(Symbols::FunctionArguments & args) {
    if (args.size() < 2 || args.size() > 3) {
        throw std::invalid_argument("Imagick::write expects (string filename [, int quality])");
    }
    Magick::Image &   image    = imageFor(args, "write");
    const std::string filename = args[1];
    if (args.size() == 3) {
        if (args[2] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("Imagick::write: quality must be an integer 1-100");
        }
        const int q = args[2];
        if (q < 1 || q > 100) {
            throw std::runtime_error("Imagick::write: quality must be 1-100");
        }
        image.quality(static_cast<size_t>(q));
    }
    image.write(filename);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::mode(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::mode missing argument: (string $mode)");
    }
    Magick::Image & image = imageFor(args, "mode");

    const std::string mode = args[1];
    if (mode == "RGB") {
        image.colorSpace(Magick::RGBColorspace);
    } else if (mode == "CMYK") {
        image.colorSpace(Magick::CMYKColorspace);
    } else if (mode == "GRAY") {
        image.colorSpace(Magick::GRAYColorspace);
    } else {
        throw std::invalid_argument("Imagick::mode: invalid mode. Supported modes are: RGB, CMYK, GRAY");
    }

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::blur(Symbols::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::invalid_argument("Imagick::blur missing argument: (double radius, double sigma)");
    }
    Magick::Image & image = imageFor(args, "blur");

    const double radius = args[1];
    const double sigma  = args[2];
    image.blur(radius, sigma);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::rotate(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::rotate missing argument: (double degrees)");
    }
    Magick::Image & image = imageFor(args, "rotate");

    const double degrees = args[1];
    image.rotate(degrees);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::flip(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::flip missing argument: (string direction)");
    }
    Magick::Image & image = imageFor(args, "flip");

    const std::string direction = args[1];
    if (direction == "horizontal") {
        image.flip();
    } else if (direction == "vertical") {
        image.flop();
    } else {
        throw std::invalid_argument("Imagick::flip: invalid direction. Supported directions are: horizontal, vertical");
    }

    return Symbols::ValuePtr::null();
}


Magick::Image & Modules::ImagickModule::imageFor(Symbols::FunctionArguments & args, const char * method,
                                                 size_t argIndex) {
    const auto & objVal = args[argIndex];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error(std::string("Imagick::") + method +
                                 (argIndex == 0 ? " must be called on an Imagick instance"
                                                : " expects an Imagick instance argument"));
    }
    const Symbols::ObjectMap & objMap = objVal->get<Symbols::ObjectMap>();
    auto                       idIt   = objMap.find("__image_id__");
    if (idIt == objMap.end() || idIt->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error(std::string("Imagick::") + method + ": no valid image - call read() first");
    }
    const int handle = idIt->second->get<int>();
    auto      imgIt  = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error(std::string("Imagick::") + method + ": image handle " +
                                 std::to_string(handle) + " is invalid");
    }
    return imgIt->second;
}

// getPixel(x, y) -> { int red, int green, int blue, int alpha }, each 0-255.
// Magick::Quantum depth varies by build (8 or 16 bit), so scale rather than assume.
Symbols::ValuePtr Modules::ImagickModule::getPixel(Symbols::FunctionArguments & args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::INTEGER ||
        args[2] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::getPixel expects (int x, int y)");
    }
    Magick::Image & image = imageFor(args, "getPixel");

    const int x = args[1];
    const int y = args[2];
    if (x < 0 || y < 0 || static_cast<size_t>(x) >= image.columns() || static_cast<size_t>(y) >= image.rows()) {
        throw std::runtime_error("Imagick::getPixel: coordinates (" + std::to_string(x) + ", " +
                                 std::to_string(y) + ") are outside the image");
    }

    const Magick::ColorRGB colour(image.pixelColor(x, y));
    Symbols::ObjectMap     out;
    out["red"]   = Symbols::ValuePtr(static_cast<int>(colour.red() * 255.0 + 0.5));
    out["green"] = Symbols::ValuePtr(static_cast<int>(colour.green() * 255.0 + 0.5));
    out["blue"]  = Symbols::ValuePtr(static_cast<int>(colour.blue() * 255.0 + 0.5));
    out["alpha"] = Symbols::ValuePtr(static_cast<int>(colour.alpha() * 255.0 + 0.5));
    return Symbols::ValuePtr(out);
}

// setPixel(x, y, red, green, blue [, alpha]), each channel 0-255.
Symbols::ValuePtr Modules::ImagickModule::setPixel(Symbols::FunctionArguments & args) {
    if (args.size() < 6 || args.size() > 7) {
        throw std::runtime_error("Imagick::setPixel expects (int x, int y, int red, int green, int blue [, int alpha])");
    }
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("Imagick::setPixel expects integer arguments");
        }
    }
    Magick::Image & image = imageFor(args, "setPixel");

    const int x = args[1];
    const int y = args[2];
    if (x < 0 || y < 0 || static_cast<size_t>(x) >= image.columns() || static_cast<size_t>(y) >= image.rows()) {
        throw std::runtime_error("Imagick::setPixel: coordinates (" + std::to_string(x) + ", " +
                                 std::to_string(y) + ") are outside the image");
    }

    const auto channel = [](int v, const char * what) {
        if (v < 0 || v > 255) {
            throw std::runtime_error(std::string("Imagick::setPixel: ") + what + " must be 0-255");
        }
        return static_cast<double>(v) / 255.0;
    };

    // Writing needs the image to own its pixels outright, otherwise the change can be
    // lost to copy-on-write sharing with another Image referencing the same blob.
    image.modifyImage();

    Magick::ColorRGB colour(channel(args[3], "red"), channel(args[4], "green"), channel(args[5], "blue"));
    if (args.size() == 7) {
        const double a = channel(args[6], "alpha");
        // An image loaded from a format without an alpha channel has nowhere to store
        // one, and the write is silently discarded. Turn the channel on first.
        if (a < 1.0 && !image.alpha()) {
            image.alpha(true);
        }
        colour.alpha(a);
    } else {
        colour.alpha(1.0);
    }
    image.pixelColor(x, y, colour);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::getWidth(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getWidth takes no arguments");
    }
    Magick::Image & image = imageFor(args, "getWidth");
    return Symbols::ValuePtr(static_cast<int>(image.columns()));
}

Symbols::ValuePtr Modules::ImagickModule::getHeight(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getHeight takes no arguments");
    }
    Magick::Image & image = imageFor(args, "getHeight");

    return Symbols::ValuePtr(static_cast<int>(image.rows()));
}

Symbols::ValuePtr Modules::ImagickModule::composite(Symbols::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::invalid_argument("Imagick::composite missing arguments: (Imagick source, int x, int y)");
    }
    if (args[2] != Symbols::Variables::Type::INTEGER || args[3] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite expects (Imagick source, int x, int y)");
    }
    Magick::Image & target = imageFor(args, "composite");
    Magick::Image & source = imageFor(args, "composite", 1);  // source is argument 1

    const int x = args[2];
    const int y = args[3];

    // The target owns its pixels before an in-place composite, otherwise the change can
    // be lost to copy-on-write sharing with another Image over the same blob.
    target.modifyImage();
    target.composite(source, x, y, Magick::OverCompositeOp);

    return Symbols::ValuePtr::null();
}

// newImage(width, height, colorHex) -> create a solid-color canvas from nothing.
// Stamps a fresh handle on the object, mirroring read().
Symbols::ValuePtr Modules::ImagickModule::newImage(Symbols::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::runtime_error("Imagick::newImage expects (int width, int height, string colorHex)");
    }
    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::newImage must be called on an Imagick instance");
    }
    if (args[1] != Symbols::Variables::Type::INTEGER || args[2] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::newImage expects integer width and height");
    }
    const int width  = args[1];
    const int height = args[2];
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Imagick::newImage: width and height must be positive");
    }
    const std::string colorHex = args[3];

    try {
        Magick::Image image(Magick::Geometry(static_cast<size_t>(width), static_cast<size_t>(height)),
                            Magick::Color(colorHex));
        image.backgroundColor(Magick::Color(colorHex));

        int handle      = next_image_id++;
        images_[handle] = image;

        Symbols::ValuePtr self                            = args[0];
        self->get<Symbols::ObjectMap>()["__image_id__"]   = Symbols::ValuePtr(handle);
        return self;
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::newImage failed: ") + e.what());
    }
}

// Shared body for gradient()/radialGradient(): ImageMagick renders "gradient:" and
// "radial-gradient:" pseudo-images natively, so a smooth mask (e.g. a vignette) is one
// call instead of a per-pixel loop. `pseudo` is "gradient" or "radial-gradient".
Symbols::ValuePtr Modules::ImagickModule::makeGradient(Symbols::FunctionArguments & args, const char * method,
                                                       const char * pseudo) {
    if (args.size() != 5) {
        throw std::runtime_error(std::string("Imagick::") + method +
                                 " expects (int width, int height, string startColor, string endColor)");
    }
    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error(std::string("Imagick::") + method + " must be called on an Imagick instance");
    }
    if (args[1] != Symbols::Variables::Type::INTEGER || args[2] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error(std::string("Imagick::") + method + " expects integer width and height");
    }
    const int width  = args[1];
    const int height = args[2];
    if (width <= 0 || height <= 0) {
        throw std::runtime_error(std::string("Imagick::") + method + ": width and height must be positive");
    }
    const std::string startColor = args[3];
    const std::string endColor   = args[4];

    try {
        Magick::Image image;
        image.size(Magick::Geometry(static_cast<size_t>(width), static_cast<size_t>(height)));
        image.read(std::string(pseudo) + ":" + startColor + "-" + endColor);

        int handle      = next_image_id++;
        images_[handle] = image;

        Symbols::ValuePtr self                          = args[0];
        self->get<Symbols::ObjectMap>()["__image_id__"] = Symbols::ValuePtr(handle);
        return self;
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::") + method + " failed: " + e.what());
    }
}

// gradient(width, height, startColor, endColor) -> native top-to-bottom linear gradient.
Symbols::ValuePtr Modules::ImagickModule::gradient(Symbols::FunctionArguments & args) {
    return makeGradient(args, "gradient", "gradient");
}

// radialGradient(width, height, innerColor, outerColor) -> native center-to-edge radial
// gradient; radialGradient(w, h, "#ffffff", "#000000") is a ready-made vignette mask to
// compositeMultiply() onto an image.
Symbols::ValuePtr Modules::ImagickModule::radialGradient(Symbols::FunctionArguments & args) {
    return makeGradient(args, "radialGradient", "radial-gradient");
}

// extent(width, height, xOff, yOff [, colorHex]) -> pad or crop to an exact size,
// filling any new area with the background color (or colorHex if given).
Symbols::ValuePtr Modules::ImagickModule::extent(Symbols::FunctionArguments & args) {
    if (args.size() < 5 || args.size() > 6) {
        throw std::runtime_error("Imagick::extent expects (int width, int height, int xOff, int yOff [, string colorHex])");
    }
    for (size_t i = 1; i <= 4; ++i) {
        if (args[i] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("Imagick::extent expects integer width, height, xOff and yOff");
        }
    }
    Magick::Image & image = imageFor(args, "extent");

    const int width  = args[1];
    const int height = args[2];
    const int xOff   = args[3];
    const int yOff   = args[4];
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Imagick::extent: width and height must be positive");
    }

    try {
        image.modifyImage();
        Magick::Geometry geom(static_cast<size_t>(width), static_cast<size_t>(height),
                              static_cast<ssize_t>(xOff), static_cast<ssize_t>(yOff));
        if (args.size() == 6) {
            const std::string colorHex = args[5];
            image.extent(geom, Magick::Color(colorHex));
        } else {
            image.extent(geom, image.backgroundColor());
        }
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::extent failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// addNoise(type, strength) -> add noise across the whole image in one native pass,
// avoiding a per-pixel scripted loop.
Symbols::ValuePtr Modules::ImagickModule::addNoise(Symbols::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::runtime_error("Imagick::addNoise expects (string type, double strength)");
    }
    Magick::Image & image = imageFor(args, "addNoise");
    const std::string type = args[1];
    const double strength  = (args[2] == Symbols::Variables::Type::INTEGER)
                                 ? static_cast<double>(args[2].get<int>())
                                 : args[2].get<double>();

    Magick::NoiseType noise;
    if (type == "gaussian")              noise = Magick::GaussianNoise;
    else if (type == "uniform")          noise = Magick::UniformNoise;
    else if (type == "impulse")          noise = Magick::ImpulseNoise;
    else if (type == "laplacian")        noise = Magick::LaplacianNoise;
    else if (type == "poisson")          noise = Magick::PoissonNoise;
    else if (type == "multiplicative")   noise = Magick::MultiplicativeGaussianNoise;
    else if (type == "random")           noise = MagickCore::RandomNoise;
    else {
        throw std::runtime_error("Imagick::addNoise: unknown type '" + type +
                                 "' (gaussian|uniform|impulse|laplacian|poisson|multiplicative|random)");
    }

    try {
        image.modifyImage();
        image.addNoise(noise, strength);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::addNoise failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// evaluate(op, value) -> apply an arithmetic operator to every pixel's color channels
// in one native pass (e.g. "multiply", 0.7 to darken).
Symbols::ValuePtr Modules::ImagickModule::evaluate(Symbols::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::runtime_error("Imagick::evaluate expects (string op, double value)");
    }
    Magick::Image & image = imageFor(args, "evaluate");
    const std::string op = args[1];
    const double value   = (args[2] == Symbols::Variables::Type::INTEGER)
                               ? static_cast<double>(args[2].get<int>())
                               : args[2].get<double>();

    Magick::MagickEvaluateOperator oper;
    if (op == "multiply")      oper = Magick::MultiplyEvaluateOperator;
    else if (op == "add")      oper = Magick::AddEvaluateOperator;
    else if (op == "subtract") oper = Magick::SubtractEvaluateOperator;
    else if (op == "divide")   oper = Magick::DivideEvaluateOperator;
    else if (op == "set")      oper = Magick::SetEvaluateOperator;
    else if (op == "min")      oper = Magick::MinEvaluateOperator;
    else if (op == "max")      oper = Magick::MaxEvaluateOperator;
    else {
        throw std::runtime_error("Imagick::evaluate: unknown op '" + op +
                                 "' (multiply|add|subtract|divide|set|min|max)");
    }

    // Operator-dependent value semantics, matching ImageMagick's -evaluate:
    //  - multiply / divide take a raw factor (0.7 darkens to 70%), used as-is;
    //  - add / subtract / set / min / max take an absolute level, so a normalized 0-1
    //    value is scaled up to the quantum range.
    const bool   isFactor = (oper == Magick::MultiplyEvaluateOperator ||
                             oper == Magick::DivideEvaluateOperator);
    const double rvalue   = isFactor ? value : value * static_cast<double>(QuantumRange);

    try {
        image.modifyImage();
        image.evaluate(Magick::CompositeChannels, oper, rvalue);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::evaluate failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// compositeMultiply(mask) -> multiply this image by a mask image (native, whole-image),
// the fast path for a Gaussian-mask vignette / darken.
Symbols::ValuePtr Modules::ImagickModule::compositeMultiply(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Imagick::compositeMultiply expects (Imagick mask)");
    }
    Magick::Image & target = imageFor(args, "compositeMultiply");
    Magick::Image & mask   = imageFor(args, "compositeMultiply", 1);

    try {
        target.modifyImage();
        target.composite(mask, 0, 0, Magick::MultiplyCompositeOp);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::compositeMultiply failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// compositeOp(source, op [, x, y]) -> composite another image with a named operator.
// Covers image-image arithmetic (e.g. "subtract" for unsharp = image - blurred).
Symbols::ValuePtr Modules::ImagickModule::compositeOp(Symbols::FunctionArguments & args) {
    if (args.size() < 3 || args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Imagick::compositeOp expects (Imagick source, string op [, int x, int y])");
    }
    Magick::Image &   target = imageFor(args, "compositeOp");
    Magick::Image &   source = imageFor(args, "compositeOp", 1);
    const std::string op     = args[2];
    const int         x      = (args.size() >= 4 && args[3] == Symbols::Variables::Type::INTEGER) ? args[3]->get<int>() : 0;
    const int         y      = (args.size() >= 5 && args[4] == Symbols::Variables::Type::INTEGER) ? args[4]->get<int>() : 0;

    Magick::CompositeOperator co;
    if (op == "over")            co = Magick::OverCompositeOp;
    else if (op == "multiply")   co = Magick::MultiplyCompositeOp;
    else if (op == "screen")     co = Magick::ScreenCompositeOp;
    else if (op == "add" || op == "plus")        co = Magick::PlusCompositeOp;
    else if (op == "subtract" || op == "minus")  co = Magick::MinusSrcCompositeOp;
    else if (op == "difference") co = Magick::DifferenceCompositeOp;
    else if (op == "darken")     co = Magick::DarkenCompositeOp;
    else if (op == "lighten")    co = Magick::LightenCompositeOp;
    else if (op == "overlay")    co = Magick::OverlayCompositeOp;
    else {
        throw std::runtime_error("Imagick::compositeOp: unknown op '" + op +
                                 "' (over|multiply|screen|add|subtract|difference|darken|lighten|overlay)");
    }

    try {
        target.modifyImage();
        target.composite(source, x, y, co);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::compositeOp failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// distort(method, args [, bestfit]) -> geometric distortion (barrel, perspective, ...).
Symbols::ValuePtr Modules::ImagickModule::distort(Symbols::FunctionArguments & args) {
    if (args.size() < 3 || args[1] != Symbols::Variables::Type::STRING ||
        (args[2] != Symbols::Variables::Type::OBJECT && args[2] != Symbols::Variables::Type::CLASS)) {
        throw std::runtime_error("Imagick::distort expects (string method, array args [, boolean bestfit])");
    }
    Magick::Image &   image  = imageFor(args, "distort");
    const std::string method = args[1];
    const bool        bestfit = (args.size() >= 4 && args[3] == Symbols::Variables::Type::BOOLEAN) ? args[3]->get<bool>() : false;

    Magick::DistortMethod dm;
    if (method == "barrel")             dm = Magick::BarrelDistortion;
    else if (method == "perspective")   dm = Magick::PerspectiveDistortion;
    else if (method == "srt")           dm = Magick::ScaleRotateTranslateDistortion;
    else if (method == "affine")        dm = Magick::AffineDistortion;
    else if (method == "arc")           dm = Magick::ArcDistortion;
    else if (method == "bilinear")      dm = Magick::BilinearForwardDistortion;
    else if (method == "polar")         dm = Magick::PolarDistortion;
    else if (method == "depolar")       dm = Magick::DePolarDistortion;
    else {
        throw std::runtime_error("Imagick::distort: unknown method '" + method +
                                 "' (barrel|perspective|srt|affine|arc|bilinear|polar|depolar)");
    }

    // Read the coefficient array (in index order) into a double vector.
    std::vector<double>        coeffs;
    const Symbols::ObjectMap & m = args[2]->get<Symbols::ObjectMap>();
    for (size_t i = 0;; ++i) {
        auto it = m.find(std::to_string(i));
        if (it == m.end()) {
            break;
        }
        switch (it->second->getType()) {
            case Symbols::Variables::Type::INTEGER: coeffs.push_back(static_cast<double>(it->second->get<int>())); break;
            case Symbols::Variables::Type::FLOAT:   coeffs.push_back(static_cast<double>(it->second->get<float>())); break;
            case Symbols::Variables::Type::DOUBLE:  coeffs.push_back(it->second->get<double>()); break;
            default:
                throw std::runtime_error("Imagick::distort: args must be numbers");
        }
    }
    if (coeffs.empty()) {
        throw std::runtime_error("Imagick::distort: args array is empty");
    }

    try {
        image.modifyImage();
        image.distort(dm, coeffs.size(), coeffs.data(), bestfit);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::distort failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

namespace {
Magick::ChannelType channelFromName(const std::string & name, const char * method) {
    if (name == "red")   return Magick::RedChannel;
    if (name == "green") return Magick::GreenChannel;
    if (name == "blue")  return Magick::BlueChannel;
    if (name == "alpha") return Magick::AlphaChannel;
    throw std::runtime_error(std::string("Imagick::") + method + ": unknown channel '" + name +
                             "' (red|green|blue|alpha)");
}
}  // namespace

// extractChannel(name) -> reduce this image in place to one channel as a grayscale image.
// Read the source into several instances and extract R/G/B into each to warp them apart
// (lateral chromatic aberration), then combineChannels() them back.
Symbols::ValuePtr Modules::ImagickModule::extractChannel(Symbols::FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Imagick::extractChannel expects (string channel)");
    }
    Magick::Image &     image = imageFor(args, "extractChannel");
    Magick::ChannelType ch    = channelFromName(args[1], "extractChannel");
    try {
        image.modifyImage();
        image = image.separate(ch);  // new grayscale image of just that channel
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::extractChannel failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// combineChannels(rImg, gImg, bImg) -> set this image's R/G/B from three grayscale images
// (typically produced by extractChannel and independently transformed).
Symbols::ValuePtr Modules::ImagickModule::combineChannels(Symbols::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::runtime_error("Imagick::combineChannels expects (Imagick red, Imagick green, Imagick blue)");
    }
    Magick::Image & target = imageFor(args, "combineChannels");
    Magick::Image & r      = imageFor(args, "combineChannels", 1);
    Magick::Image & g      = imageFor(args, "combineChannels", 2);
    Magick::Image & b      = imageFor(args, "combineChannels", 3);
    try {
        target.modifyImage();
        target.composite(r, 0, 0, Magick::CopyRedCompositeOp);
        target.composite(g, 0, 0, Magick::CopyGreenCompositeOp);
        target.composite(b, 0, 0, Magick::CopyBlueCompositeOp);
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::combineChannels failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}

// stripImage() -> remove all profiles and comments (EXIF, ICC, ...).
Symbols::ValuePtr Modules::ImagickModule::stripImage(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error("Imagick::stripImage takes no arguments");
    }
    Magick::Image & image = imageFor(args, "stripImage");
    try {
        image.modifyImage();
        image.strip();
    } catch (const Magick::Exception & e) {
        throw std::runtime_error(std::string("Imagick::stripImage failed: ") + e.what());
    }
    return Symbols::ValuePtr::null();
}
