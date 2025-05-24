// HeaderModule.hpp
#ifndef MODULES_HEADERMODULE_HPP
#define MODULES_HEADERMODULE_HPP

#include <string>
#include <unordered_map>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief FastCGI header management (header setting like PHP header()).
 */
class HeaderModule : public BaseModule {
  public:
    HeaderModule() { setModuleName("Header"); }
    
    void registerFunctions() override {
        std::vector<FunctParameterInfo> params = {
            { "key",   Symbols::Variables::Type::STRING, "HTTP header key"   },
            { "value", Symbols::Variables::Type::STRING, "HTTP header value" }
        };

        REGISTER_FUNCTION("header", Symbols::Variables::Type::NULL_TYPE, params,
                          "FastCGI header management (header setting like PHP header())",
                          [](const FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 || args[0]->getType() != Symbols::Variables::Type::STRING ||
                                  args[1]->getType() != Symbols::Variables::Type::STRING) {
                                  throw Exception("header(key, value) requires two string arguments");
                              }
                              const std::string & key = args[0]->get<std::string>();
                              const std::string & val = args[1]->get<std::string>();
                              setHeader(key, val);
                              return Symbols::ValuePtr::null();
                          });
    }

    /**
     * @brief Set or overwrite a header value.
     */
    static void setHeader(const std::string & key, const std::string & value) { headers_[key] = value; }

    /**
     * @brief Get all headers set during script execution.
     */
    static const std::unordered_map<std::string, std::string> & getHeaders() noexcept { return headers_; }

    /**
     * @brief Clear all previously set headers.
     */
    static void clearHeaders() noexcept { headers_.clear(); }

  private:
    // Inline static for single definition across translation units
    inline static std::unordered_map<std::string, std::string> headers_;
};

}  // namespace Modules

#endif  // MODULES_HEADERMODULE_HPP
