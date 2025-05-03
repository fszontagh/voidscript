#ifndef XML_NODULE_HPP
#define XML_NODULE_HPP
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdint.h>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

inline static std::unordered_map<int, xmlDocPtr>  docHolder;
inline static std::unordered_map<int, xmlNodePtr> nodeHolder;
static int                                        nextDoc = 1;

class XmlModule : public BaseModule {
  public:
    /**
     * @brief Register this module's symbols
     */
    void registerModule() override;

    ~XmlModule() {
        for (auto dox : docHolder) {
            xmlFreeDoc(dox.second);
        }
        docHolder.clear();
    }
  private:
    std::string       moduleName      = "Xml2";
    const std::string objectStoreName = "__xml2_handler_id__";

    // example: https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/example/parse1.c

    Symbols::Value readFile(FuncionArguments & args);
    Symbols::Value readMemory(FuncionArguments & args);
    Symbols::Value GetRootElement(FuncionArguments & args);
    Symbols::Value GetNodeAttributes(FuncionArguments & args);

};  // Class
}  // namespace Modules
#endif  // XML_NODULE_HPP
