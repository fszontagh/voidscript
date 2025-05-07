#include "XmlModule.hpp"

#include <cstring>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

void Modules::XmlModule::registerModule(IModuleContext & context) {
    // Register classes using UnifiedModuleManager macros
    REGISTER_CLASS(context, this->moduleName);
    REGISTER_CLASS(context, "XmlNode");
    REGISTER_CLASS(context, "XmlAttr");

    // Register methods using UnifiedModuleManager macros
    REGISTER_METHOD(context, this->moduleName, "readFile", &XmlModule::readFile);
    REGISTER_METHOD(context, this->moduleName, "readMemory", &XmlModule::readMemory);
    REGISTER_METHOD(context, this->moduleName, "getRootElement", &XmlModule::GetRootElement);
    REGISTER_METHOD(context, "XmlNode", "getAttributes", &XmlModule::GetNodeAttributes);
}

Symbols::Value Modules::XmlModule::readFile(FunctionArguments & args) {
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

Symbols::Value Modules::XmlModule::readMemory(FunctionArguments & args) {
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

Symbols::Value Modules::XmlModule::GetRootElement(FunctionArguments & args) {
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

Symbols::Value Modules::XmlModule::GetNodeAttributes(FunctionArguments & args) {
    auto val    = this->getObjectValue(args, "__xml_node_handler_id__");
    int  handle = val.get<int>();

    if (nodeHolder.contains(handle)) {
        const auto * node         = nodeHolder[handle];
        const auto * node_name    = node->name;
        const auto   node_type    = node->type;
        const auto * node_content = node->content;
        const auto   children     = node->children;

        Symbols::Value::ObjectMap map;
        map["tagName"]    = Symbols::Value(node_name);
        map["tagType"]    = Symbols::Value(XmlModule::xmlElementTypeToString(node_type));
        map["tagContent"] = Symbols::Value(node_content);

        if (map["tagContent"].get<std::string>().empty()) {
            map["tagContent"].setNULL();
        }

        if (children) {
            Symbols::Value::ObjectMap childrenArray;
            unsigned int              i = 0;
            for (xmlNodePtr child = children; child; child = child->next) {
                if (child->type != XML_ELEMENT_NODE) {
                    continue;
                }

                int childHandle         = nextDoc++;
                nodeHolder[childHandle] = child;

                Symbols::Value::ObjectMap childObj =
                    this->storeObject(args, Symbols::Value{ childHandle }, "__xml_node_handler_id__");
                childObj["__class__"] = "XmlNode";

                //childrenArray.push_back(Symbols::Value::makeClassInstance(childObj));
                childrenArray[std::to_string(i++)] = Symbols::Value::makeClassInstance(childObj);
            }

            if (!childrenArray.empty()) {
                map["children"] = childrenArray;
            } else {
                map["children"] = Symbols::Value::makeNull(Symbols::Variables::Type::OBJECT);
            }
        } else {
            map["children"] = Symbols::Value::makeNull(Symbols::Variables::Type::OBJECT);
        }

        return map;
    }

    throw std::runtime_error(this->moduleName + ":getNodeAttributes: invalid handler");
}
