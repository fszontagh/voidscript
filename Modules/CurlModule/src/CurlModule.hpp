// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef CURLMODULE_CURLMODULE_HPP
#define CURLMODULE_CURLMODULE_HPP

#include "Modules/BaseModule.hpp"
#include <vector>
#include "Symbols/Value.hpp"

namespace Modules {

class CurlModule : public BaseModule {
public:
    /**
     * @brief Register this module's symbols (HTTP GET and POST functions).
     */
    void registerModule() override;
    
    /**
     * @brief Perform HTTP GET: curlGet(url [, options])
     * options is an object with optional fields:
     *   timeout (int or double seconds),
     *   follow_redirects (bool),
     *   headers (object mapping header names to values)
     */
    Symbols::Value curlGet(const std::vector<Symbols::Value>& args);
    
    /**
     * @brief Perform HTTP POST: curlPost(url, data [, options])
     * options is an object with optional fields:
     *   timeout (int or double seconds),
     *   follow_redirects (bool),
     *   headers (object mapping header names to values)
     */
    Symbols::Value curlPost(const std::vector<Symbols::Value>& args);
};

} // namespace Modules

#endif // CURLMODULE_CURLMODULE_HPP
