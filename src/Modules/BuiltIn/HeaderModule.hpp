// HeaderModule.hpp
#ifndef MODULES_HEADERMODULE_HPP
#define MODULES_HEADERMODULE_HPP

#include <string>
#include <unordered_map>
#include <algorithm>
#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief FastCGI header management (header setting like PHP header()).
 */
class HeaderModule : public BaseModule {
  public:
    void registerModule() override {
        auto &mgr = ModuleManager::instance();
        mgr.registerFunction("header", [](FunctionArguments &args) {
            if (args.size() != 2 || args[0].getType() != Symbols::Variables::Type::STRING ||
                args[1].getType() != Symbols::Variables::Type::STRING) {
                throw Exception("header(key, value) requires two string arguments");
            }
            const std::string &key = args[0].get<std::string>();
            const std::string &val = args[1].get<std::string>();
            setHeader(key, val);
            return Symbols::Value();
        });
    }

    /**
     * @brief Set or overwrite a header value.
     */
    static void setHeader(const std::string &key, const std::string &value) {
        headers_[key] = value;
    }

    /**
     * @brief Get all headers set during script execution.
     */
    static const std::unordered_map<std::string, std::string> &getHeaders() noexcept {
        return headers_;
    }

    /**
     * @brief Clear all previously set headers.
     */
    static void clearHeaders() noexcept {
        headers_.clear();
    }

  private:
    // Inline static for single definition across translation units
    inline static std::unordered_map<std::string, std::string> headers_;
};

} // namespace Modules

#endif // MODULES_HEADERMODULE_HPP