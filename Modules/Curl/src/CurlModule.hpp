// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef CURLMODULE_HPP
#define CURLMODULE_HPP

#include <curl/curl.h>

#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class CurlClient {
  private:
    CURL *              curl;
    std::string         response;
    struct curl_slist * headers;
    int                 timeoutSec      = 20;
    bool                followRedirects = false;
    bool                initialized     = false;

    static size_t write_callback(void * ptr, size_t size, size_t nmemb, void * userdata);

  public:
    CurlClient();
    ~CurlClient();

    void setUrl(const std::string & url);
    void setTimeout(int seconds);
    void setFollowRedirects(bool follow);
    void setHeaders(Symbols::ValuePtr headersObj);
    void addHeader(const std::string & name, const std::string & value);
    void clearHeaders();

    std::string get(const std::string & url, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    std::string post(const std::string & url, const std::string & data, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    std::string put(const std::string & url, const std::string & data, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    std::string delete_(const std::string & url, Symbols::ValuePtr options = Symbols::ValuePtr::null());

  private:
    void        initialize();
    void        cleanup();
    void        setCommonOptions();
    void        parseOptions(Symbols::ValuePtr options);
    std::string performRequest();
};

/**
 *
 */
class CurlModule final : public BaseModule {
  public:
    CurlModule() { setModuleName("Curl"); }

    /**
     * @brief Register this module's symbols (HTTP GET and POST functions).
     */
    void registerFunctions() override;

    /**
     * @brief Perform HTTP GET: curlGet(url [, options])
     * options is an object with optional fields:
     *   timeout (int or double seconds),
     *   follow_redirects (bool),
     *   headers (object mapping header names to values)
     */
    static Symbols::ValuePtr curlGet(FunctionArguments & args);

    /**
     * @brief Perform HTTP POST: curlPost(url, data [, options])
     * options is an object with optional fields:
     *   timeout (int or double seconds),
     *   follow_redirects (bool),
     *   headers (object mapping header names to values)
     */
    static Symbols::ValuePtr curlPost(FunctionArguments & args);

    Symbols::ValuePtr curlPut(FunctionArguments & args);
    Symbols::ValuePtr curlDelete(FunctionArguments & args);
};

}  // namespace Modules

#endif  // CURLMODULE_HPP
