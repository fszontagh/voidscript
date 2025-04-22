#ifndef XML_NODULE_HPP
#define XML_NODULE_HPP
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class XmlModule : public BaseModule {
  public:
    /**
     * @brief Register this module's symbols
     */
    void registerModule() override;

  private:
    // example: https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/example/parse1.c
    inline Symbols::Value constructor(FuncionArguments & args) {
        Symbols::Value returnValue = Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
        return returnValue;
    }

    inline Symbols::Value createDoc(FuncionArguments & args) {
        Symbols::Value returnValue = Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
        return returnValue;
    }
};  // Class
}  // namespace Modules
#endif  // XML_NODULE_HPP
