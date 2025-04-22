#include "ImagickModule.hpp"

#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

void Modules::ImagickModule::registerModule() {
    auto & registry = Symbols::ClassRegistry::instance();
    registry.registerClass("Imagick");
    registry.addMethod("Imagick", "read");
    registry.addMethod("Imagick", "crop");
    registry.addMethod("Imagick", "resize");
    registry.addMethod("Imagick", "write");

    auto & mgr = ModuleManager::instance();
    mgr.registerFunction(
        "Imagick::read", [this](const std::vector<Symbols::Value> & args) { return this->read(args); },
        Symbols::Variables::Type::CLASS);

    mgr.registerFunction(
        "Imagick::crop", [this](const std::vector<Symbols::Value> & args) { return this->crop(args); },
        Symbols::Variables::Type::NULL_TYPE);
    mgr.registerFunction(
        "Imagick::write", [this](const std::vector<Symbols::Value> & args) { return this->write(args); },
        Symbols::Variables::Type::NULL_TYPE);

        mgr.registerFunction(
            "Imagick::resize", [this](const std::vector<Symbols::Value> & args) { return this->resize(args); },
            Symbols::Variables::Type::NULL_TYPE);
}

Symbols::Value Modules::ImagickModule::read(FuncionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Imagick::read expects (filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[0].getType() != Symbols::Variables::Type::CLASS && args[0].getType() != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::read must be called on Imagick instance");
    }

    auto        objMap   = std::get<Symbols::Value::ObjectMap>(args[0].get());
    // Connection parameters
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

Symbols::Value Modules::ImagickModule::crop(Modules::FuncionArguments & args) {
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
        throw std::runtime_error("Imagick::cropt: no valid image");
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

Symbols::Value Modules::ImagickModule::resize(Modules::FuncionArguments & args) {
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

Symbols::Value Modules::ImagickModule::write(Modules::FuncionArguments & args) {
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
