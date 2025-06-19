// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef CURLMODULE_HPP
#define CURLMODULE_HPP

#include <curl/curl.h>

#include <string>
#include <unordered_map>
#include <memory>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

// Enhanced response structure for detailed HTTP information
struct CurlResponseData {
    int statusCode = 0;
    std::string body = "";
    Symbols::ObjectMap headers = {};
    double totalTime = 0.0;
    bool success = false;
    std::string errorMessage = "";
};

class CurlClient {
  private:
    CURL *              curl;
    std::string         response;
    struct curl_slist * headers;
    struct curl_slist * response_headers;
    int                 timeoutSec      = 20;
    bool                followRedirects = false;
    bool                initialized     = false;

    static size_t write_callback(void * ptr, size_t size, size_t nmemb, void * userdata);
    static size_t header_callback(void * ptr, size_t size, size_t nmemb, void * userdata);

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

    // Enhanced methods that return detailed response information
    CurlResponseData getDetailed(const std::string & url, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    CurlResponseData postDetailed(const std::string & url, const std::string & data, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    CurlResponseData putDetailed(const std::string & url, const std::string & data, Symbols::ValuePtr options = Symbols::ValuePtr::null());
    CurlResponseData deleteDetailed(const std::string & url, Symbols::ValuePtr options = Symbols::ValuePtr::null());

  private:
    void        initialize();
    void        cleanup();
    void        setCommonOptions();
    void        parseOptions(Symbols::ValuePtr options);
    std::string performRequest();
    CurlResponseData performDetailedRequest();
    Symbols::ObjectMap parseResponseHeaders(const std::string& header_data);
    std::string responseHeaderData;
};

// CurlResponse wrapper class for VoidScript OOP interface
class CurlResponseWrapper {
  private:
    static std::unordered_map<std::string, CurlResponseData> response_data_map_;

  public:
    static Symbols::ValuePtr construct(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getStatusCode(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getBody(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getHeaders(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getHeader(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getTotalTime(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr isSuccess(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getErrorMessage(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr asJson(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr toString(Symbols::FunctionArguments& args);

    // Factory method for creating response objects
    static Symbols::ValuePtr createResponse(const CurlResponseData& responseData);
};

// CurlClient wrapper class for VoidScript OOP interface
class CurlClientWrapper {
  private:
    static std::unordered_map<std::string, std::unique_ptr<CurlClient>> client_map_;
    static std::unordered_map<std::string, std::string> base_url_map_;
    static std::unordered_map<std::string, Symbols::ObjectMap> default_headers_map_;
    static std::unordered_map<std::string, int> timeout_map_;
    static std::unordered_map<std::string, bool> follow_redirects_map_;

  public:
    // Constructors
    static Symbols::ValuePtr constructDefault(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr constructWithBaseUrl(Symbols::FunctionArguments& args);

    // Configuration methods (fluent API)
    static Symbols::ValuePtr setBaseUrl(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr setTimeout(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr setDefaultHeader(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr setFollowRedirects(Symbols::FunctionArguments& args);

    // HTTP methods
    static Symbols::ValuePtr get(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr getWithOptions(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr post(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr postWithOptions(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr put(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr putWithOptions(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr delete_(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr deleteWithOptions(Symbols::FunctionArguments& args);

  private:
    static CurlClient* getOrCreateClient(const std::string& objectId);
    static std::string buildFullUrl(const std::string& objectId, const std::string& url);
    static Symbols::ValuePtr mergeOptions(const std::string& objectId, Symbols::ValuePtr options);
};

/**
 * Curl Module providing both legacy functions and OOP interface
 */
class CurlModule final : public BaseModule {
  public:
    CurlModule() {
        setModuleName("Curl");
        setDescription("Provides HTTP client functionality using libcurl, supporting GET, POST, PUT, and DELETE requests with customizable headers, timeouts, and redirect handling. Includes both legacy functions and modern OOP interface with CurlClient and CurlResponse classes.");
    }

    /**
     * @brief Register this module's symbols (legacy functions and OOP classes).
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

  private:
    void registerLegacyFunctions();
    void registerOOPClasses();
};

}  // namespace Modules

#endif  // CURLMODULE_HPP
