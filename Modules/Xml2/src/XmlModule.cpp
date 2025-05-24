#include "XmlModule.hpp"

#include <cstring>
#include <iostream>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

void Modules::XmlModule::registerModule() {
    // Register classes using UnifiedModuleManager macros
    REGISTER_CLASS(this->className);
    REGISTER_CLASS("XmlNode");
    REGISTER_CLASS("XmlAttr");

    // Register methods for Xml2 class
    std::vector<Modules::FunctParameterInfo> params = {
        { "filename", Symbols::Variables::Type::STRING, "The path to the XML file to read" }
    };

    REGISTER_METHOD(
        this->className, "readFile", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->readFile(args); },
        Symbols::Variables::Type::CLASS, "Read XML from a file");

    params = {
        { "string", Symbols::Variables::Type::STRING, "The XML content as a string to parse" }
    };

    REGISTER_METHOD(
        this->className, "readMemory", params,
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->readMemory(args); },
        Symbols::Variables::Type::CLASS, "Read XML from a string");

    REGISTER_METHOD(
        this->className, "getRootElement", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->GetRootElement(args); },
        Symbols::Variables::Type::CLASS, "Get the root element of the XML document");

    // Register methods for XmlNode class
    REGISTER_METHOD(
        "XmlNode", "getAttributes", {},
        [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->GetNodeAttributes(args); },
        Symbols::Variables::Type::OBJECT, "Get the attributes of an XML node");

    // Register properties for Xml2 class
    REGISTER_PROPERTY(this->className, "__xml2_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
    REGISTER_PROPERTY("XmlNode", "__xml_node_handler_id__", Symbols::Variables::Type::INTEGER, nullptr);
}

Symbols::ValuePtr Modules::XmlModule::readFile(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error(this->className +
                                 " expects one parameter (string $filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[1] != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->className + "::readFile: invalid parameter, must be string");
    }
    int handler = nextDoc++;

    const std::string filename = args[1];
    xmlDocPtr         doc      = xmlReadFile(filename.c_str(), NULL, 0);
    docHolder[handler]         = doc;
    Symbols::ObjectMap objMap  = this->storeObject(args, Symbols::ValuePtr(handler), this->objectStoreName);
    objMap["__class__"]        = this->className;
    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr Modules::XmlModule::readMemory(FunctionArguments & args) {
    if (args.size() != 2 && args.size() != 3 && args.size() != 4) {
        throw std::runtime_error(
            this->className +
            " expects one parameter (string $xmlcontent, int $size = -1, string $basename = \"noname.xml\"), "
            "got: " +
            std::to_string(args.size() - 1));
    }

    if (args[1] != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument(this->className + "::readMemory: invalid first parameter, must be string");
    }

    std::string content  = args[1];
    std::string basename = "noname.xml";
    int         size     = -1;

    if (args.size() >= 3) {
        if (args[2] != Symbols::Variables::Type::INTEGER) {
            throw std::invalid_argument(this->className + "::readMemory: size parameter must be integer");
        }
        size = args[2];
    }

    if (args.size() == 4) {
        if (args[3] != Symbols::Variables::Type::STRING) {
            throw std::invalid_argument(this->className + "::readmemory: basename parameter must be string");
        }
        basename = args[3].get<std::string>();
    }

    int       handler = nextDoc++;
    xmlDocPtr doc     = xmlReadMemory(content.c_str(), size == -1 ? content.size() : size, basename.c_str(), NULL, 0);
    if (doc == NULL) {
        throw std::runtime_error(this->className + "::readMemory: failed to parse XML");
    }
    docHolder[handler] = doc;

    // Create a new object map for the XML document
    Symbols::ObjectMap objMap;
    if (args.size() > 0 &&
        (args[0] == Symbols::Variables::Type::CLASS || args[0] == Symbols::Variables::Type::OBJECT)) {
        // If this is a method call on an existing object, use its map
        objMap = args[0];
    }

    // Use the new property management system
    auto & manager = UnifiedModuleManager::instance();
    manager.setObjectProperty(this->className, "__xml2_handler_id__", handler);
    manager.setObjectProperty(this->className, "__class__", this->className);
    manager.setObjectProperty(this->className, "__type__", this->className);

    // Copy properties to the object map for backward compatibility
    objMap["__xml2_handler_id__"] = manager.getObjectProperty(this->className, "__xml2_handler_id__");
    objMap["__class__"]           = manager.getObjectProperty(this->className, "__class__");
    objMap["__type__"]            = manager.getObjectProperty(this->className, "__type__");

    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr Modules::XmlModule::GetRootElement(const FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error("Xml2::getRootElement: expected 1 argument");
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Xml2::getRootElement: invalid object type");
    }

    auto & manager = UnifiedModuleManager::instance();
    if (!manager.hasObjectProperty(this->className, "__xml2_handler_id__")) {
        throw std::runtime_error("Xml2::getRootElement: invalid object");
    }

    int handlerId = manager.getObjectProperty(this->className, "__xml2_handler_id__");
    if (handlerId == -1) {
        throw std::runtime_error("Xml2::getRootElement: invalid object");
    }

    auto docIt = docHolder.find(handlerId);
    if (docIt == docHolder.end()) {
        throw std::runtime_error("Xml2::getRootElement: document not found");
    }

    // Get the root element
    xmlNodePtr root = xmlDocGetRootElement(docHolder[handlerId]);
    if (root == NULL) {
        throw std::runtime_error(this->className + "::getRootElement: invalid root");
    }

    // Store the root node and create a new object for it
    int nodeHandle         = nextDoc++;
    nodeHolder[nodeHandle] = root;

    // Create a new object map for the XML node
    Symbols::ObjectMap nodeObjMap;

    // Use the new property management system for the XmlNode class
    manager.setObjectProperty("XmlNode", "__xml_node_handler_id__", nodeHandle);
    manager.setObjectProperty("XmlNode", "__class__", "XmlNode");

    // Copy properties to the object map for backward compatibility
    nodeObjMap["__xml_node_handler_id__"] = manager.getObjectProperty("XmlNode", "__xml_node_handler_id__");
    nodeObjMap["__class__"]               = manager.getObjectProperty("XmlNode", "__class__");

    return Symbols::ValuePtr::makeClassInstance(nodeObjMap);
}

Symbols::ValuePtr Modules::XmlModule::GetNodeAttributes(FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error(this->className + "::getNodeAttributes: must be called with no arguments");
    }
    auto val    = this->getObjectValue(args, "__xml_node_handler_id__");
    int  handle = val;

    if (nodeHolder.contains(handle)) {
        const auto * node         = nodeHolder[handle];
        const auto * node_name    = node->name;
        const auto   node_type    = node->type;
        const auto * node_content = node->content;
        const auto   children     = node->children;

        Symbols::ObjectMap map;
        map["tagName"]    = Symbols::ValuePtr(node_name);
        map["tagType"]    = Symbols::ValuePtr(XmlModule::xmlElementTypeToString(node_type));
        map["tagContent"] = Symbols::ValuePtr(node_content);


        if (children) {
            Symbols::ObjectMap childrenArray;
            unsigned int       i = 0;
            for (xmlNodePtr child = children; child; child = child->next) {
                if (child->type != XML_ELEMENT_NODE) {
                    continue;
                }

                int childHandle         = nextDoc++;
                nodeHolder[childHandle] = child;

                Symbols::ObjectMap childObj = this->storeObject(args, childHandle, "__xml_node_handler_id__");
                childObj["__class__"]       = "XmlNode";

                //childrenArray.push_back(Symbols::ValuePtr::makeClassInstance(childObj));
                childrenArray[std::to_string(i++)] = Symbols::ValuePtr::makeClassInstance(childObj);
            }

            if (!childrenArray.empty()) {
                map["children"] = childrenArray;
            } else {
                map["children"] = Symbols::ValuePtr::null(Symbols::Variables::Type::OBJECT);
            }
        } else {
            map["children"] = Symbols::ValuePtr::null(Symbols::Variables::Type::OBJECT);
        }

        return map;
    }

    throw std::runtime_error(this->className + "::getNodeAttributes: invalid handler");
}
