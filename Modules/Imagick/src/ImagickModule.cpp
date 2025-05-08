#include "ImagickModule.hpp"

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

void Modules::ImagickModule::registerModule() {
    std::vector<FunctParameterInfo> params = {
        { "filename", Symbols::Variables::Type::STRING },
    };

    REGISTER_CLASS(this->name());

    REGISTER_METHOD(
        this->name(), "read", params, [this](const std::vector<Symbols::Value> & args) { return this->read(args); },
        Symbols::Variables::Type::CLASS, "Read an image file");

    REGISTER_METHOD(
        this->name(), "write", params, [this](const std::vector<Symbols::Value> & args) { return this->write(args); },
        Symbols::Variables::Type::NULL_TYPE, "Save the image");

    params = {
        { "width",   Symbols::Variables::Type::INTEGER },
        { "height",  Symbols::Variables::Type::INTEGER },
        { "xOffset", Symbols::Variables::Type::INTEGER },
        { "yOffset", Symbols::Variables::Type::INTEGER },
    };

    REGISTER_METHOD(
        this->name(), "crop", params, [this](const std::vector<Symbols::Value> & args) { return this->crop(args); },
        Symbols::Variables::Type::NULL_TYPE, "Crop the image");

    params = {
        { "width",  Symbols::Variables::Type::INTEGER },
        { "height", Symbols::Variables::Type::INTEGER }
    };

    REGISTER_METHOD(
        this->name(), "resize", params, [this](const std::vector<Symbols::Value> & args) { return this->resize(args); },
        Symbols::Variables::Type::NULL_TYPE, "Resize an image");

    //params = {
    //    { "mode", Symbols::Variables::Type::STRING },
    //};

    //    REGISTER_METHOD(
    //        this->name(), "mode",params, [this](const std::vector<Symbols::Value> & args) { return this->mode(args); },
    //        Symbols::Variables::Type::NULL_TYPE, "Change image mode");

    params = {
        { "radius", Symbols::Variables::Type::DOUBLE },
        { "sigma",  Symbols::Variables::Type::DOUBLE },
    };

    REGISTER_METHOD(
        this->name(), "blur", params, [this](const std::vector<Symbols::Value> & args) { return this->blur(args); },
        Symbols::Variables::Type::NULL_TYPE, "Blur an image");

    params = {
        { "degrees", Symbols::Variables::Type::DOUBLE },
    };

    REGISTER_METHOD(
        this->name(), "rotate", params, [this](const std::vector<Symbols::Value> & args) { return this->rotate(args); },
        Symbols::Variables::Type::NULL_TYPE, "Rotate image");

    REGISTER_METHOD(
        this->name(), "flip", params, [this](const std::vector<Symbols::Value> & args) { return this->flip(args); },
        Symbols::Variables::Type::NULL_TYPE, "Flip image");
    REGISTER_METHOD(
        this->name(), "getWidth", {}, [this](const std::vector<Symbols::Value> & args) { return this->getWidth(args); },
        Symbols::Variables::Type::INTEGER, "Get the width of the image");
    REGISTER_METHOD(
        this->name(), "getHeight", {},
        [this](const std::vector<Symbols::Value> & args) { return this->getHeight(args); },
        Symbols::Variables::Type::INTEGER, "Get the height of the image");
    //REGISTER_METHOD(
    //    this->name(), "composite", [this](const std::vector<Symbols::Value> & args) { return this->composite(args); },
    //    Symbols::Variables::Type::NULL_TYPE, "Composite image");
}

Symbols::Value Modules::ImagickModule::read(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Imagick::read expects (filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[0].getType() != Symbols::Variables::Type::CLASS && args[0].getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::read must be called on Imagick instance");
    }

    auto objMap = std::get<Symbols::Value::ObjectMap>(args[0].get());

    std::string filename = Symbols::Value::to_string(args[1].get());

    if (!std::filesystem::exists(filename)) {
        throw std::invalid_argument("File does not exists: " + filename);
    }
    Magick::Image image;
    image.read(filename);
    int handle             = next_image_id++;
    images_[handle]        = image;
    objMap["__image_id__"] = Symbols::Value(handle);
    return Symbols::Value::makeClassInstance(objMap);
}

Symbols::Value Modules::ImagickModule::crop(Modules::FunctionArguments & args) {
    if (args.size() != 5) {
        throw std::invalid_argument(
            "Imagick::crop missing argument: (int width, int height, int xOffset, int , int yOffset)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::crop must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::crop: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::crop: image not found");
    }

    const int width   = args[1].get<int>();
    const int height  = args[2].get<int>();
    const int xOffset = args[3].get<int>();
    const int yOffset = args[4].get<int>();

    imgIt->second.crop({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::resize(Modules::FunctionArguments & args) {
    if (args.size() < 2) {
        throw std::invalid_argument(
            "Imagick::crop missing argument: (string $sizes | int $width, int $height, int $xOffset = 0, int $yOffset "
            "= 0)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::resize must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::resoze: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::resize: image not found");
    }

    if (args[1].getType() == Symbols::Variables::Type::STRING) {
        const std::string size = args[1].get<std::string>();
        imgIt->second.resize(size);
        return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
    }

    int yOffset = 0;
    int xOffset = 0;
    if (args.size() < 3) {
        throw std::invalid_argument("Imagick::resize: Missing arguments");
    }
    if (args.size() == 5) {
        xOffset = args[3].get<int>();
        yOffset = args[4].get<int>();
    }

    const int width  = args[1].get<int>();
    const int height = args[2].get<int>();

    imgIt->second.resize({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::write(Modules::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::write missing argument: (string $filename)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::write must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::write: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::write: image not found");
    }
    std::string filename = args[1].get<std::string>();
    imgIt->second.write(filename);

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::mode(Modules::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::mode missing argument: (string $mode)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::mode must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::mode: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::mode: image not found");
    }

    std::string mode = args[1].get<std::string>();
    if (mode == "RGB") {
        imgIt->second.colorSpace(Magick::RGBColorspace);
    } else if (mode == "CMYK") {
        imgIt->second.colorSpace(Magick::CMYKColorspace);
    } else if (mode == "GRAY") {
        imgIt->second.colorSpace(Magick::GRAYColorspace);
    } else {
        throw std::invalid_argument("Imagick::mode: invalid mode. Supported modes are: RGB, CMYK, GRAY");
    }

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::blur(Modules::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::invalid_argument("Imagick::blur missing argument: (double radius, double sigma)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::blur must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::blur: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::blur: image not found");
    }

    const double radius = args[1].get<double>();
    const double sigma  = args[2].get<double>();
    imgIt->second.blur(radius, sigma);

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::rotate(Modules::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::rotate missing argument: (double degrees)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::rotate must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::rotate: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::rotate: image not found");
    }

    const double degrees = args[1].get<double>();
    imgIt->second.rotate(degrees);

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::flip(Modules::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::flip missing argument: (string direction)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::flip must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::flip: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::flip: image not found");
    }

    std::string direction = args[1].get<std::string>();
    if (direction == "horizontal") {
        imgIt->second.flip();
    } else if (direction == "vertical") {
        imgIt->second.flop();
    } else {
        throw std::invalid_argument("Imagick::flip: invalid direction. Supported directions are: horizontal, vertical");
    }

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::getWidth(Modules::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getWidth takes no arguments");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::getWidth must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::getWidth: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::getWidth: image not found");
    }

    return Symbols::Value(static_cast<int>(imgIt->second.columns()));
}

Symbols::Value Modules::ImagickModule::getHeight(Modules::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getHeight takes no arguments");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::getHeight must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::getHeight: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::getHeight: image not found");
    }

    return Symbols::Value(static_cast<int>(imgIt->second.rows()));
}

Symbols::Value Modules::ImagickModule::composite(Modules::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::invalid_argument("Imagick::composite missing arguments: (Imagick source, int x, int y)");
    }
    const auto & objVal = args[0];
    if (objVal.getType() != Symbols::Variables::Type::CLASS && objVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::composite must be called on Imagick instance");
    }
    auto objMap   = std::get<Symbols::Value::ObjectMap>(objVal.get());
    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite: no valid image");
    }

    handle     = itHandle->second.get<int>();
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::composite: image not found");
    }

    // Forrás kép lekérése
    const auto & sourceVal = args[1];
    if (sourceVal.getType() != Symbols::Variables::Type::CLASS &&
        sourceVal.getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::composite: source must be an Imagick instance");
    }
    auto sourceMap    = std::get<Symbols::Value::ObjectMap>(sourceVal.get());
    auto sourceHandle = sourceMap.find("__image_id__");
    if (sourceHandle == sourceMap.end() || sourceHandle->second.getType() != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite: no valid source image");
    }

    int  sourceImageHandle = sourceHandle->second.get<int>();
    auto sourceImgIt       = images_.find(sourceImageHandle);
    if (sourceImgIt == images_.end()) {
        throw std::runtime_error("Imagick::composite: source image not found");
    }

    const int x = args[2].get<int>();
    const int y = args[3].get<int>();

    imgIt->second.composite(sourceImgIt->second, x, y, Magick::OverCompositeOp);

    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}
