#include "XmlModule.hpp"

#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

void Modules::XmlModule::registerModule() {
    auto & registry = Symbols::ClassRegistry::instance();
    registry.registerClass(this->moduleName);
    registry.registerClass("XmlNode");
    registry.registerClass("XmlAttr");

    registry.addMethod(this->moduleName, "readFile",
                       [this](const std::vector<Symbols::Value> & args) { return this->readFile(args); });
    registry.addMethod(this->moduleName, "readMemory",
                       [this](const std::vector<Symbols::Value> & args) { return this->readMemory(args); });

    registry.addMethod(
        this->moduleName, "getRootElement",
        [this](const std::vector<Symbols::Value> & args) { return this->GetRootElement(args); },
        Symbols::Variables::Type::CLASS);

    registry.addMethod(
        "XmlNode", "getAttributes", [this](FuncionArguments & args) { return this->GetNodeAttributes(args); },
        Symbols::Variables::Type::OBJECT);
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

Symbols::Value Modules::XmlModule::GetRootElement(FuncionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error(this->moduleName + "::getRootElement: must be called with no arguments");
    }
    auto objMap   = this->getObjectMap(args, "getRootElement");
    int  handle   = 0;
    auto itHandle = objMap.find(this->objectStoreName);
    if (itHandle == objMap.end()) {
        throw std::runtime_error(this->moduleName + "::getRootElement: no handle found");
    }
    handle          = itHandle->second.get<int>();
    xmlNodePtr root = xmlDocGetRootElement(docHolder[handle]);
    if (root == NULL) {
        throw std::runtime_error(this->moduleName + "::getRootElement: invalid root");
    }
    nodeHolder[handle] = root;
    auto obj           = this->storeObject(args, Symbols::Value{ handle }, "__xml_node_handler_id__");
    obj["__class__"]   = "XmlNode";  // add the class too, to make it callable
    return Symbols::Value::makeClassInstance(obj);
}

Symbols::Value Modules::XmlModule::GetNodeAttributes(FuncionArguments & args) {
    auto val    = this->getObjectValue(args, "__xml_node_handler_id__");
    int  handle = val.get<int>();

    if (nodeHolder.contains(handle)) {
        const auto * const node_name    = nodeHolder[handle]->name;
        const auto         node_type    = nodeHolder[handle]->type;
        const auto *       node_content = nodeHolder[handle]->content;


        Symbols::Value::ObjectMap map;
        map["name"]    = Symbols::Value(node_name);
        map["type"]    = Symbols::Value(node_type);
        map["content"] = Symbols::Value(node_content);
        return map;
    }
    throw std::runtime_error(this->moduleName + ":getNodeAttributes: invalid handler");
}
