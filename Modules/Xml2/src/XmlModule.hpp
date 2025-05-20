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
    XmlModule() {
        setModuleName("Xml2");
        this->className = "Xml2";
        LIBXML_TEST_VERSION
        xmlInitParser();
    }

    /**
     * @brief Register this module's symbols
     */
    void registerModule() override;

    ~XmlModule() {
        for (auto dox : docHolder) {
            xmlFreeDoc(dox.second);
        }
        docHolder.clear();
        nodeHolder.clear();
        xmlCleanupParser();
    }

    static std::string xmlElementTypeToString(int type) {
        std::unordered_map<int, std::string> xmlElementTypeMap = {
            { 1,  "XML_ELEMENT_NODE"       },
            { 2,  "XML_ATTRIBUTE_NODE"     },
            { 3,  "XML_TEXT_NODE"          },
            { 4,  "XML_CDATA_SECTION_NODE" },
            { 5,  "XML_ENTITY_REF_NODE"    },
            { 6,  "XML_ENTITY_NODE"        },
            { 7,  "XML_PI_NODE"            },
            { 8,  "XML_COMMENT_NODE"       },
            { 9,  "XML_DOCUMENT_NODE"      },
            { 10, "XML_DOCUMENT_TYPE_NODE" },
            { 11, "XML_DOCUMENT_FRAG_NODE" },
            { 12, "XML_NOTATION_NODE"      },
            { 13, "XML_HTML_DOCUMENT_NODE" },
            { 14, "XML_DTD_NODE"           },
            { 15, "XML_ELEMENT_DECL"       },
            { 16, "XML_ATTRIBUTE_DECL"     },
            { 17, "XML_ENTITY_DECL"        },
            { 18, "XML_NAMESPACE_DECL"     },
            { 19, "XML_XINCLUDE_START"     },
            { 20, "XML_XINCLUDE_END"       },
#ifdef LIBXML_DOCB_ENABLED
            { 21, "XML_DOCB_DOCUMENT_NODE" },
#endif
        };
        const auto it = xmlElementTypeMap.find(type);
        if (it == xmlElementTypeMap.end()) {
            return "Unknown";
        }
        return it->second;
    }

  private:
    const std::string objectStoreName = "__xml2_handler_id__";
    std::string       className;

    // example: https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/example/parse1.c

    Symbols::ValuePtr readFile(FunctionArguments & args);
    Symbols::ValuePtr readMemory(FunctionArguments & args);
    Symbols::ValuePtr GetRootElement(const FunctionArguments & args);
    Symbols::ValuePtr GetNodeAttributes(FunctionArguments & args);

};  // Class
}  // namespace Modules
#endif  // XML_NODULE_HPP
