#include "XmlModule.hpp"

#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

void Modules::XmlModule::registerModule() {
    auto & registry = Symbols::ClassRegistry::instance();
    registry.registerClass("Xml2");

    registry.addMethod("Xml2", "createDOC",
                       [this](const std::vector<Symbols::Value> & args) { return this->createDoc(args); });
}
