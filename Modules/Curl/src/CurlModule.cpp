// CurlModule implementation: HTTP GET and POST using libcurl
#include "CurlModule.hpp"

#include <curl/curl.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <sstream>

#include "Modules/BuiltIn/JsonModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

// Static member definitions
std::unordered_map<std::string, CurlResponseData> CurlResponseWrapper::response_data_map_;
std::unordered_map<std::string, std::unique_ptr<CurlClient>> CurlClientWrapper::client_map_;
std::unordered_map<std::string, std::string> CurlClientWrapper::base_url_map_;
std::unordered_map<std::string, Symbols::ObjectMap> CurlClientWrapper::default_headers_map_;
std::unordered_map<std::string, int> CurlClientWrapper::timeout_map_;
std::unordered_map<std::string, bool> CurlClientWrapper::follow_redirects_map_;

// CurlClient implementation
size_t CurlClient::write_callback(void * ptr, size_t size, size_t nmemb, void * userdata) {
    auto * buffer = static_cast<std::string *>(userdata);
    buffer->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

size_t CurlClient::header_callback(void * ptr, size_t size, size_t nmemb, void * userdata) {
    auto * buffer = static_cast<std::string *>(userdata);
    buffer->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

CurlClient::CurlClient() : curl(nullptr), headers(nullptr), response_headers(nullptr), timeoutSec(0), followRedirects(false), initialized(false) {
    initialize();
}

CurlClient::~CurlClient() {
    cleanup();
}

void CurlClient::initialize() {
    if (!initialized) {
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("curl: failed to initialize");
        }
        initialized = true;
    }
}

void CurlClient::cleanup() {
    if (headers) {
        curl_slist_free_all(headers);
        headers = nullptr;
    }
    if (response_headers) {
        curl_slist_free_all(response_headers);
        response_headers = nullptr;
    }
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
    initialized = false;
}

void CurlClient::setUrl(const std::string & url) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
}

void CurlClient::setTimeout(int seconds) {
    timeoutSec = seconds;
    if (timeoutSec > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    }
}

void CurlClient::setFollowRedirects(bool follow) {
    followRedirects = follow;
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow ? 1L : 0L);
}

void CurlClient::setHeaders(Symbols::ValuePtr headersObj) {
    clearHeaders();
    if (headersObj != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("headers must be an object");
    }

    const Symbols::ObjectMap hobj = headersObj;
    for (const auto & hk : hobj) {
        if (hk.second != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("header values must be string");
        }

        // Get key and handle quoted keys
        std::string key = hk.first;
        if (key.size() >= 2 && key.front() == '"' && key.back() == '"') {
            key = key.substr(1, key.size() - 2);
        }

        std::string line = key + ": " + hk.second.get<std::string>();
        headers          = curl_slist_append(headers, line.c_str());
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
}

void CurlClient::addHeader(const std::string & name, const std::string & value) {
    std::string line = name + ": " + value;
    headers          = curl_slist_append(headers, line.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

void CurlClient::clearHeaders() {
    if (headers) {
        curl_slist_free_all(headers);
        headers = nullptr;
    }
}

void CurlClient::setCommonOptions() {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
}

void CurlClient::parseOptions(Symbols::ValuePtr options) {
    if (!options || options->isNULL()) {
        return; // No options to parse
    }
    
    if (options != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("options must be an object");
    }

    // Preprocess all keys to remove quotes
    Symbols::ObjectMap cleanedOptions;
    Symbols::ObjectMap obj = options;

    for (const auto & kv : obj) {
        std::string key = kv.first;

        // Remove quotes if they exist
        if (key.size() >= 2 && key.front() == '"' && key.back() == '"') {
            key = key.substr(1, key.size() - 2);
        }

        // Store the cleaned key with the original value
        cleanedOptions[key] = kv.second;
    }

    // Now process with the cleaned keys
    for (const auto & kv : cleanedOptions) {
        const std::string & key = kv.first;
        const auto          v   = kv.second;

        if (key == "timeout") {
            setTimeout(v);
        } else if (key == "follow_redirects" || key == "follow") {
            if (v != Symbols::Variables::Type::BOOLEAN) {
                throw std::runtime_error("follow_redirects must be boolean");
            }
            setFollowRedirects(v);
        } else if (key == "headers") {
            setHeaders(v);
        } else {
            std::string err = "unknown option '" + key + "'";
            throw std::runtime_error(err);
        }
    }
}

std::string CurlClient::performRequest() {
    response.clear();
    setCommonOptions();
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("curl: request failed: " + std::string(curl_easy_strerror(res)));
    }
    return response;
}

std::string CurlClient::get(const std::string & url, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(std::move(options));
    return performRequest();
}

std::string CurlClient::post(const std::string & url, const std::string & data, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(std::move(options));
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    return performRequest();
}

std::string CurlClient::put(const std::string & url, const std::string & data, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(options);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    return performRequest();
}

std::string CurlClient::delete_(const std::string & url, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(options);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    return performRequest();
}

// Enhanced methods that return detailed response information
CurlResponseData CurlClient::getDetailed(const std::string & url, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(std::move(options));
    return performDetailedRequest();
}

CurlResponseData CurlClient::postDetailed(const std::string & url, const std::string & data, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(std::move(options));
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    return performDetailedRequest();
}

CurlResponseData CurlClient::putDetailed(const std::string & url, const std::string & data, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(options);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    return performDetailedRequest();
}

CurlResponseData CurlClient::deleteDetailed(const std::string & url, Symbols::ValuePtr options) {
    setUrl(url);
    parseOptions(options);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    return performDetailedRequest();
}

CurlResponseData CurlClient::performDetailedRequest() {
    response.clear();
    responseHeaderData.clear();
    setCommonOptions();
    
    // Set up header callback
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaderData);
    
    CURLcode res = curl_easy_perform(curl);
    
    CurlResponseData responseData;
    responseData.body = response;
    
    if (res == CURLE_OK) {
        // Get status code
        long statusCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        responseData.statusCode = static_cast<int>(statusCode);
        
        // Get total time
        double totalTime = 0.0;
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totalTime);
        responseData.totalTime = totalTime;
        
        // Parse headers
        responseData.headers = parseResponseHeaders(responseHeaderData);
        
        // Determine success
        responseData.success = (statusCode >= 200 && statusCode < 300);
        responseData.errorMessage = "";
    } else {
        responseData.statusCode = 0;
        responseData.success = false;
        responseData.errorMessage = curl_easy_strerror(res);
        responseData.totalTime = 0.0;
    }
    
    return responseData;
}

Symbols::ObjectMap CurlClient::parseResponseHeaders(const std::string& header_data) {
    Symbols::ObjectMap headers;
    
    std::istringstream stream(header_data);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Skip status line and empty lines
        if (line.empty() || line.find("HTTP/") == 0) {
            continue;
        }
        
        // Find colon separator
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            headers[name] = Symbols::ValuePtr(value);
        }
    }
    
    return headers;
}

// CurlResponseWrapper implementation
Symbols::ValuePtr CurlResponseWrapper::construct(Symbols::FunctionArguments& args) {
    if (args.size() != 1) {
        throw std::runtime_error("CurlResponse::construct expects no parameters");
    }
    
    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlResponse::construct must be called on CurlResponse instance");
    }
    
    // Initialize with empty response data
    std::string objectId = args[0].toString();
    response_data_map_[objectId] = CurlResponseData{};
    
    return args[0];
}

Symbols::ValuePtr CurlResponseWrapper::getStatusCode(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getStatusCode: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.statusCode);
}

Symbols::ValuePtr CurlResponseWrapper::getBody(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getBody: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.body);
}

Symbols::ValuePtr CurlResponseWrapper::getHeaders(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getHeaders: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.headers, true); // true for object type
}

Symbols::ValuePtr CurlResponseWrapper::getHeader(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlResponse::getHeader expects one string argument");
    }
    
    std::string objectId = args[0].toString();
    std::string headerName = args[1].get<std::string>();
    
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getHeader: object not properly initialized");
    }
    
    auto headerIt = it->second.headers.find(headerName);
    if (headerIt != it->second.headers.end()) {
        return headerIt->second;
    }
    
    return Symbols::ValuePtr("");
}

Symbols::ValuePtr CurlResponseWrapper::getTotalTime(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getTotalTime: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.totalTime);
}

Symbols::ValuePtr CurlResponseWrapper::isSuccess(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::isSuccess: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.success);
}

Symbols::ValuePtr CurlResponseWrapper::getErrorMessage(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::getErrorMessage: object not properly initialized");
    }
    return Symbols::ValuePtr(it->second.errorMessage);
}

Symbols::ValuePtr CurlResponseWrapper::asJson(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::asJson: object not properly initialized");
    }
    
    try {
        // Use VoidScript's built-in JSON parsing functionality
        std::vector<Symbols::ValuePtr> jsonArgs = { Symbols::ValuePtr(it->second.body) };
        Symbols::FunctionArguments jsonFuncArgs(jsonArgs);
        
        // Create a JsonModule instance to parse the JSON
        // Note: This is a simplified approach - in a real implementation,
        // you might want to use the actual JsonModule registration
        Symbols::ObjectMap result;
        // For now, return empty object - JSON parsing would need JsonModule integration
        return Symbols::ValuePtr(result, true);
    } catch (const std::exception& e) {
        throw std::runtime_error("CurlResponse::asJson: Failed to parse JSON: " + std::string(e.what()));
    }
}

Symbols::ValuePtr CurlResponseWrapper::toString(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = response_data_map_.find(objectId);
    if (it == response_data_map_.end()) {
        throw std::runtime_error("CurlResponse::toString: object not properly initialized");
    }
    
    std::ostringstream oss;
    oss << "CurlResponse{statusCode=" << it->second.statusCode
        << ", success=" << (it->second.success ? "true" : "false")
        << ", totalTime=" << it->second.totalTime << "s"
        << ", bodyLength=" << it->second.body.length() << "}";
    return Symbols::ValuePtr(oss.str());
}

Symbols::ValuePtr CurlResponseWrapper::createResponse(const CurlResponseData& responseData) {
    // Create a new CurlResponse object
    Symbols::ObjectMap objectMap;
    objectMap["__class__"] = Symbols::ValuePtr("CurlResponse");
    
    // Create the object
    auto responseObj = Symbols::ValuePtr::makeClassInstance(objectMap);
    std::string objectId = responseObj.toString();
    
    // Store the response data
    response_data_map_[objectId] = responseData;
    
    return responseObj;
}

// CurlClientWrapper implementation
Symbols::ValuePtr CurlClientWrapper::constructDefault(Symbols::FunctionArguments& args) {
    if (args.size() != 1) {
        throw std::runtime_error("CurlClient::construct expects no parameters");
    }
    
    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlClient::construct must be called on CurlClient instance");
    }
    
    std::string objectId = args[0].toString();
    
    // Initialize client state
    client_map_[objectId] = std::make_unique<CurlClient>();
    base_url_map_[objectId] = "";
    default_headers_map_[objectId] = Symbols::ObjectMap{};
    timeout_map_[objectId] = 30;
    follow_redirects_map_[objectId] = true;
    
    // Return the original object
    return args[0];
}

Symbols::ValuePtr CurlClientWrapper::constructWithBaseUrl(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::construct expects one string parameter (baseUrl)");
    }
    
    std::string objectId = args[0].toString();
    std::string baseUrl = args[1].get<std::string>();
    
    // Initialize client state
    client_map_[objectId] = std::make_unique<CurlClient>();
    base_url_map_[objectId] = baseUrl;
    default_headers_map_[objectId] = Symbols::ObjectMap{};
    timeout_map_[objectId] = 30;
    follow_redirects_map_[objectId] = true;
    
    return args[0];
}

Symbols::ValuePtr CurlClientWrapper::setBaseUrl(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::setBaseUrl expects one string argument");
    }
    
    std::string objectId = args[0].toString();
    std::string baseUrl = args[1].get<std::string>();
    
    base_url_map_[objectId] = baseUrl;
    
    return args[0]; // Return self for chaining
}

Symbols::ValuePtr CurlClientWrapper::setTimeout(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("CurlClient::setTimeout expects one integer argument");
    }
    
    std::string objectId = args[0].toString();
    int timeout = args[1].get<int>();
    
    timeout_map_[objectId] = timeout;
    
    return args[0]; // Return self for chaining
}

Symbols::ValuePtr CurlClientWrapper::setDefaultHeader(Symbols::FunctionArguments& args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING || args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::setDefaultHeader expects two string arguments");
    }
    
    std::string objectId = args[0].toString();
    std::string name = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    
    default_headers_map_[objectId][name] = Symbols::ValuePtr(value);
    
    return args[0]; // Return self for chaining
}

Symbols::ValuePtr CurlClientWrapper::setFollowRedirects(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::BOOLEAN) {
        throw std::runtime_error("CurlClient::setFollowRedirects expects one boolean argument");
    }
    
    std::string objectId = args[0].toString();
    bool follow = args[1].get<bool>();
    
    follow_redirects_map_[objectId] = follow;
    
    return args[0]; // Return self for chaining
}

Symbols::ValuePtr CurlClientWrapper::get(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::get expects one string argument (url)");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr options = mergeOptions(objectId, Symbols::ValuePtr::null());
    
    CurlResponseData responseData = client->getDetailed(fullUrl, options);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::getWithOptions(Symbols::FunctionArguments& args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING || args[2] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlClient::get expects string (url) and object (options) arguments");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    Symbols::ValuePtr options = args[2];
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr mergedOptions = mergeOptions(objectId, options);
    
    CurlResponseData responseData = client->getDetailed(fullUrl, mergedOptions);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::post(Symbols::FunctionArguments& args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING || args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::post expects two string arguments (url, data)");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    std::string data = args[2].get<std::string>();
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr options = mergeOptions(objectId, Symbols::ValuePtr::null());
    
    CurlResponseData responseData = client->postDetailed(fullUrl, data, options);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::postWithOptions(Symbols::FunctionArguments& args) {
    if (args.size() != 4 || args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING || args[3] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlClient::post expects string (url), string (data) and object (options) arguments");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    std::string data = args[2].get<std::string>();
    Symbols::ValuePtr options = args[3];
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr mergedOptions = mergeOptions(objectId, options);
    
    CurlResponseData responseData = client->postDetailed(fullUrl, data, mergedOptions);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::put(Symbols::FunctionArguments& args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING || args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::put expects two string arguments (url, data)");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    std::string data = args[2].get<std::string>();
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr options = mergeOptions(objectId, Symbols::ValuePtr::null());
    
    CurlResponseData responseData = client->putDetailed(fullUrl, data, options);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::putWithOptions(Symbols::FunctionArguments& args) {
    if (args.size() != 4 || args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING || args[3] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlClient::put expects string (url), string (data) and object (options) arguments");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    std::string data = args[2].get<std::string>();
    Symbols::ValuePtr options = args[3];
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr mergedOptions = mergeOptions(objectId, options);
    
    CurlResponseData responseData = client->putDetailed(fullUrl, data, mergedOptions);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::delete_(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("CurlClient::delete expects one string argument (url)");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr options = mergeOptions(objectId, Symbols::ValuePtr::null());
    
    CurlResponseData responseData = client->deleteDetailed(fullUrl, options);
    return CurlResponseWrapper::createResponse(responseData);
}

Symbols::ValuePtr CurlClientWrapper::deleteWithOptions(Symbols::FunctionArguments& args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING || args[2] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("CurlClient::delete expects string (url) and object (options) arguments");
    }
    
    std::string objectId = args[0].toString();
    std::string url = args[1].get<std::string>();
    Symbols::ValuePtr options = args[2];
    
    CurlClient* client = getOrCreateClient(objectId);
    std::string fullUrl = buildFullUrl(objectId, url);
    Symbols::ValuePtr mergedOptions = mergeOptions(objectId, options);
    
    CurlResponseData responseData = client->deleteDetailed(fullUrl, mergedOptions);
    return CurlResponseWrapper::createResponse(responseData);
}

CurlClient* CurlClientWrapper::getOrCreateClient(const std::string& objectId) {
    auto it = client_map_.find(objectId);
    if (it == client_map_.end()) {
        client_map_[objectId] = std::make_unique<CurlClient>();
    }
    return client_map_[objectId].get();
}

std::string CurlClientWrapper::buildFullUrl(const std::string& objectId, const std::string& url) {
    auto baseIt = base_url_map_.find(objectId);
    if (baseIt != base_url_map_.end() && !baseIt->second.empty()) {
        // If URL is already absolute, return as-is
        if (url.find("http://") == 0 || url.find("https://") == 0) {
            return url;
        }
        // Combine base URL with relative URL
        std::string baseUrl = baseIt->second;
        if (baseUrl.back() == '/' && url.front() == '/') {
            return baseUrl + url.substr(1);
        } else if (baseUrl.back() != '/' && url.front() != '/') {
            return baseUrl + "/" + url;
        } else {
            return baseUrl + url;
        }
    }
    return url;
}

Symbols::ValuePtr CurlClientWrapper::mergeOptions(const std::string& objectId, Symbols::ValuePtr options) {
    Symbols::ObjectMap mergedOptions;
    
    // Add default configuration
    auto timeoutIt = timeout_map_.find(objectId);
    if (timeoutIt != timeout_map_.end()) {
        mergedOptions["timeout"] = Symbols::ValuePtr(timeoutIt->second);
    }
    
    auto followIt = follow_redirects_map_.find(objectId);
    if (followIt != follow_redirects_map_.end()) {
        mergedOptions["follow_redirects"] = Symbols::ValuePtr(followIt->second);
    }
    
    auto headersIt = default_headers_map_.find(objectId);
    if (headersIt != default_headers_map_.end() && !headersIt->second.empty()) {
        mergedOptions["headers"] = Symbols::ValuePtr(headersIt->second, true);
    }
    
    // Merge with provided options
    if (options && !options->isNULL() && options == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap providedOptions = options;
        for (const auto& kv : providedOptions) {
            mergedOptions[kv.first] = kv.second;
        }
    }
    
    return Symbols::ValuePtr(mergedOptions, true);
}

// CurlModule implementation
void CurlModule::registerFunctions() {
    // Register legacy functions for backward compatibility
    registerLegacyFunctions();
    
    // Register new OOP classes
    registerOOPClasses();
}

void CurlModule::registerLegacyFunctions() {
    std::vector<Symbols::FunctionParameterInfo> param_list = {
        { "url", Symbols::Variables::Type::STRING, "The URL to send the request to" },
        { "options", Symbols::Variables::Type::OBJECT, "Additional options for the request", true }
    };

    REGISTER_FUNCTION("curlGet", Symbols::Variables::Type::STRING, param_list, "Perform HTTP GET",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->curlGet(args); });

    REGISTER_FUNCTION("curlDelete", Symbols::Variables::Type::STRING, param_list, "Perform HTTP DELETE",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->curlDelete(args); });
    param_list = {
        { "url", Symbols::Variables::Type::STRING, "URL to perform query" },
        { "data", Symbols::Variables::Type::STRING, "Data to send" },
        { "options", Symbols::Variables::Type::STRING, "Optional CURL options object", true }
    };

    REGISTER_FUNCTION("curlPost", Symbols::Variables::Type::STRING, param_list, "Perform HTTP POST",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr { return this->curlPost(args); });

    REGISTER_FUNCTION("curlPut", Symbols::Variables::Type::STRING, param_list, "Perform HTTP PUT",
                      [this](FunctionArguments & args) -> Symbols::ValuePtr { return this->curlPut(args); });
}

void CurlModule::registerOOPClasses() {
    // Register CurlResponse class
    REGISTER_CLASS("CurlResponse");
    
    // Note: Properties are managed internally through response_data_map_
    // rather than using REGISTER_PROPERTY
    
    // Register CurlResponse methods
    std::vector<Symbols::FunctionParameterInfo> no_params = {};
    REGISTER_METHOD("CurlResponse", "construct", no_params, CurlResponseWrapper::construct,
                   Symbols::Variables::Type::CLASS, "Create new CurlResponse");
    
    REGISTER_METHOD("CurlResponse", "getStatusCode", no_params, CurlResponseWrapper::getStatusCode,
                   Symbols::Variables::Type::INTEGER, "Get HTTP status code");
    
    REGISTER_METHOD("CurlResponse", "getBody", no_params, CurlResponseWrapper::getBody,
                   Symbols::Variables::Type::STRING, "Get response body");
    
    REGISTER_METHOD("CurlResponse", "getHeaders", no_params, CurlResponseWrapper::getHeaders,
                   Symbols::Variables::Type::OBJECT, "Get all response headers");
    
    std::vector<Symbols::FunctionParameterInfo> header_param = {
        {"name", Symbols::Variables::Type::STRING, "Header name"}
    };
    REGISTER_METHOD("CurlResponse", "getHeader", header_param, CurlResponseWrapper::getHeader,
                   Symbols::Variables::Type::STRING, "Get specific header value");
    
    REGISTER_METHOD("CurlResponse", "getTotalTime", no_params, CurlResponseWrapper::getTotalTime,
                   Symbols::Variables::Type::DOUBLE, "Get total request time");
    
    REGISTER_METHOD("CurlResponse", "isSuccess", no_params, CurlResponseWrapper::isSuccess,
                   Symbols::Variables::Type::BOOLEAN, "Check if request was successful");
    
    REGISTER_METHOD("CurlResponse", "getErrorMessage", no_params, CurlResponseWrapper::getErrorMessage,
                   Symbols::Variables::Type::STRING, "Get error message if any");
    
    REGISTER_METHOD("CurlResponse", "asJson", no_params, CurlResponseWrapper::asJson,
                   Symbols::Variables::Type::OBJECT, "Parse body as JSON object");
    
    REGISTER_METHOD("CurlResponse", "toString", no_params, CurlResponseWrapper::toString,
                   Symbols::Variables::Type::STRING, "Get string representation");

    // Register CurlClient class
    REGISTER_CLASS("CurlClient");
    
    // Note: Properties are managed internally through static maps
    // rather than using REGISTER_PROPERTY
    
    // Register CurlClient constructors
    REGISTER_METHOD("CurlClient", "construct", no_params, CurlClientWrapper::constructDefault,
                   Symbols::Variables::Type::CLASS, "Create new CurlClient");
    
    std::vector<Symbols::FunctionParameterInfo> baseurl_param = {
        {"baseUrl", Symbols::Variables::Type::STRING, "Base URL for requests"}
    };
    REGISTER_METHOD("CurlClient", "constructWithBaseUrl", baseurl_param, CurlClientWrapper::constructWithBaseUrl,
                   Symbols::Variables::Type::CLASS, "Create CurlClient with base URL");
    
    // Register CurlClient configuration methods (fluent API)
    std::vector<Symbols::FunctionParameterInfo> url_param = {
        {"url", Symbols::Variables::Type::STRING, "Base URL to set"}
    };
    REGISTER_METHOD("CurlClient", "setBaseUrl", url_param, CurlClientWrapper::setBaseUrl,
                   Symbols::Variables::Type::CLASS, "Set base URL and return self for chaining");
    
    std::vector<Symbols::FunctionParameterInfo> timeout_param = {
        {"seconds", Symbols::Variables::Type::INTEGER, "Timeout in seconds"}
    };
    REGISTER_METHOD("CurlClient", "setTimeout", timeout_param, CurlClientWrapper::setTimeout,
                   Symbols::Variables::Type::CLASS, "Set timeout and return self for chaining");
    
    std::vector<Symbols::FunctionParameterInfo> header_params = {
        {"name", Symbols::Variables::Type::STRING, "Header name"},
        {"value", Symbols::Variables::Type::STRING, "Header value"}
    };
    REGISTER_METHOD("CurlClient", "setDefaultHeader", header_params, CurlClientWrapper::setDefaultHeader,
                   Symbols::Variables::Type::CLASS, "Set default header and return self for chaining");
    
    std::vector<Symbols::FunctionParameterInfo> follow_param = {
        {"follow", Symbols::Variables::Type::BOOLEAN, "Whether to follow redirects"}
    };
    REGISTER_METHOD("CurlClient", "setFollowRedirects", follow_param, CurlClientWrapper::setFollowRedirects,
                   Symbols::Variables::Type::CLASS, "Set redirect behavior and return self for chaining");
    
    // Register CurlClient HTTP methods
    std::vector<Symbols::FunctionParameterInfo> get_params = {
        {"url", Symbols::Variables::Type::STRING, "URL to request"}
    };
    REGISTER_METHOD("CurlClient", "get", get_params, CurlClientWrapper::get,
                   Symbols::Variables::Type::CLASS, "Perform GET request");
    
    std::vector<Symbols::FunctionParameterInfo> post_params = {
        {"url", Symbols::Variables::Type::STRING, "URL to request"},
        {"data", Symbols::Variables::Type::STRING, "Data to send"}
    };
    REGISTER_METHOD("CurlClient", "post", post_params, CurlClientWrapper::post,
                   Symbols::Variables::Type::CLASS, "Perform POST request");
    
    REGISTER_METHOD("CurlClient", "put", post_params, CurlClientWrapper::put,
                   Symbols::Variables::Type::CLASS, "Perform PUT request");
    
    REGISTER_METHOD("CurlClient", "delete", get_params, CurlClientWrapper::delete_,
                   Symbols::Variables::Type::CLASS, "Perform DELETE request");
}

Symbols::ValuePtr CurlModule::curlGet(FunctionArguments & args) {
    if (args.empty() || args.size() > 2) {
        throw std::runtime_error("curlGet: expects url and optional options object");
    }
    CurlClient        client;
    const std::string url     = args[0];
    Symbols::ValuePtr options = Symbols::ValuePtr::null();

    if (args.size() == 2) {
        options = args[1];
    }
    return Symbols::ValuePtr(client.get(url, options));
}

Symbols::ValuePtr CurlModule::curlPost(FunctionArguments & args) {
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error("curlPost: expects url, data, and optional options object");
    }
    CurlClient client;

    const std::string url     = args[0];
    const std::string data    = args[1];
    Symbols::ValuePtr options = Symbols::ValuePtr::null();

    if (args.size() == 3) {
        options = args[2];
    }

    return Symbols::ValuePtr(client.post(url, data, options));
}

Symbols::ValuePtr CurlModule::curlPut(FunctionArguments & args) {
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error("curlPut: expects url, data, and optional options object");
    }
    CurlClient client;

    const std::string url     = args[0];
    const std::string data    = args[1];
    Symbols::ValuePtr options = Symbols::ValuePtr::null();

    if (args.size() == 3) {
        options = args[2];
    }

    return Symbols::ValuePtr(client.put(url, data, options));
}

Symbols::ValuePtr CurlModule::curlDelete(FunctionArguments & args) {
    if (args.empty() || args.size() > 2) {
        throw std::runtime_error("curlDelete: expects url and optional options object");
    }
    CurlClient        client;
    const std::string url     = args[0];
    Symbols::ValuePtr options = Symbols::ValuePtr::null();

    if (args.size() == 2) {
        options = args[1];
    }

    return Symbols::ValuePtr(client.delete_(url, options));
}

}  // namespace Modules
