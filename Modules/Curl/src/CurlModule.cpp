// CurlModule implementation: HTTP GET and POST using libcurl
#include "CurlModule.hpp"

#include <curl/curl.h>

#include <stdexcept>
#include <string>
#include <utility>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

// CurlClient implementation
size_t CurlClient::write_callback(void * ptr, size_t size, size_t nmemb, void * userdata) {
    auto * buffer = static_cast<std::string *>(userdata);
    buffer->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

CurlClient::CurlClient() : curl(nullptr), headers(nullptr), timeoutSec(0), followRedirects(false), initialized(false) {
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
    if (options->isNULL() || options != Symbols::Variables::Type::OBJECT) {
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

// CurlModule implementation
void CurlModule::registerFunctions() {
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
        options = args[1];
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
        options = args[1];
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

    if (args.size() == 3) {
        options = args[1];
    }

    return Symbols::ValuePtr(client.delete_(url, options));
}

}  // namespace Modules
