#include "XmlModule.hpp"

#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

void Modules::XmlModule::registerModule() {
    auto & registry = Symbols::ClassRegistry::instance();
    registry.registerClass(this->moduleName);

    registry.addMethod(this->moduleName, "readFile",
                       [this](const std::vector<Symbols::Value> & args) { return this->readFile(args); });
}

Symbols::Value Modules::XmlModule::readFile(FuncionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("XML2 expects one parameter (string $filename), got: " +
                                 std::to_string(args.size() - 1));
    }

    if (args[1].getType() != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->moduleName + "::readFile: invalid parameter, must be string");
    }
    int handler = nextDoc++;

    const std::string filename       = args[1].get<std::string>();
    xmlDocPtr         doc            = xmlReadFile(filename.c_str(), NULL, 0);
    docHolder[handler]               = doc;
    Symbols::Value::ObjectMap objMap = this->storeObject(args, Symbols::Value{ handler }, this->objectStoreName);
    return Symbols::Value::makeClassInstance(objMap);
}

Symbols::Value Modules::XmlModule::readMemory(FuncionArguments & args) {
    if (args.size() < 2) {
        throw std::runtime_error(
            "XML2 expects one parameter (string $xmlcontent, int $size = -1, string $basename = \"noname.xml\"), "
            "got: " +
            std::to_string(args.size() - 1));
    }
    std::string content;
    std::string basename = "noname.xml";
    int         size     = -1;

    if (args[1].getType() != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->moduleName + "::readMemory: invalid first parameter, must be string");
    }
    content = args[1].get<std::string>();
    if (args.size() == 3) {
        if (args[2].getType() != Symbols::Variables::Type::INTEGER) {
            throw std::invalid_argument(this->moduleName + "::readMemory: size parameter must be integer");
        }
        size = args[2].get<int>();
    }
    if (args.size() == 4) {
        if (args[3].getType() != Symbols::Variables::Type::STRING) {
            throw std::invalid_argument(this->moduleName + "::readmemory: basename parameter must be string");
        }
        basename = args[3].get<std::string>();
    }
    int handler = nextDoc++;

    xmlDocPtr doc      = xmlReadMemory(content.c_str(), size == -1 ? content.size() : size, basename.c_str(), NULL, 0);
    docHolder[handler] = doc;
    Symbols::Value::ObjectMap objMap = this->storeObject(args, Symbols::Value{ handler }, this->objectStoreName);
    return Symbols::Value::makeClassInstance(objMap);
}
