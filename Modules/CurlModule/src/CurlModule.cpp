// CurlModule implementation: HTTP GET and POST using libcurl
#include "CurlModule.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

// Callback for libcurl to write received data into a std::string
static size_t write_callback(void * ptr, size_t size, size_t nmemb, void * userdata) {
    auto * buffer = static_cast<std::string *>(userdata);
    buffer->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

// Register module functions
void Modules::CurlModule::registerModule() {
    auto & mgr = Modules::ModuleManager::instance();
    // Register HTTP GET: curlGet(url)
    mgr.registerFunction("curlGet", [this](FuncionArguments & args) -> Symbols::Value { return this->curlGet(args); });
    // Register HTTP POST: curlPost(url, data)
    mgr.registerFunction("curlPost",
                         [this](FuncionArguments & args) -> Symbols::Value { return this->curlPost(args); });
}

Symbols::Value Modules::CurlModule::curlPost(FuncionArguments & args) {
    // curlPost: url, data [, options]
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error("curlPost: expects url, data, and optional options object");
    }
    std::string         url             = Symbols::Value::to_string(args[0]);
    std::string         data            = Symbols::Value::to_string(args[1]);
    struct curl_slist * headers         = nullptr;
    long                timeoutSec      = 0;
    bool                follow          = false;
    bool                haveContentType = false;
    if (args.size() == 3) {
        using namespace Symbols;
        if (args[2].getType() != Variables::Type::OBJECT) {
            throw std::runtime_error("curlPost: options must be object");
        }
        const auto & obj = std::get<Value::ObjectMap>(args[2].get());
        for (const auto & kv : obj) {
            const std::string & key = kv.first;
            const Value &       v   = kv.second;
            if (key == "timeout") {
                using namespace Variables;
                switch (v.getType()) {
                    case Type::INTEGER:
                        timeoutSec = v.get<int>();
                        break;
                    case Type::DOUBLE:
                        timeoutSec = static_cast<long>(v.get<double>());
                        break;
                    case Type::FLOAT:
                        timeoutSec = static_cast<long>(v.get<float>());
                        break;
                    default:
                        throw std::runtime_error("curlPost: timeout must be number");
                }
            } else if (key == "follow_redirects") {
                if (v.getType() != Variables::Type::BOOLEAN) {
                    throw std::runtime_error("curlPost: follow_redirects must be boolean");
                }
                follow = v.get<bool>();
            } else if (key == "headers") {
                if (v.getType() != Variables::Type::OBJECT) {
                    throw std::runtime_error("curlPost: headers must be object");
                }
                const auto & hobj = std::get<Value::ObjectMap>(v.get());
                for (const auto & hk : hobj) {
                    if (hk.second.getType() != Variables::Type::STRING) {
                        throw std::runtime_error("curlPost: header values must be string");
                    }
                    std::string hdr   = hk.first + ": " + hk.second.get<std::string>();
                    std::string lower = hk.first;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower == "content-type") {
                        haveContentType = true;
                    }
                    headers = curl_slist_append(headers, hdr.c_str());
                }
            } else {
                throw std::runtime_error("curlPost: unknown option '" + key + "'");
            }
        }
    }
    CURL * curl = curl_easy_init();
    if (!curl) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        throw std::runtime_error("curl: failed to initialize");
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (timeoutSec > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    }
    if (follow) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    if (!haveContentType) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (res != CURLE_OK) {
        std::string error = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl: request failed: " + error);
    }
    curl_easy_cleanup(curl);
    return Symbols::Value(response);
}

Symbols::Value Modules::CurlModule::curlGet(FuncionArguments & args) {
    // curlGet: url [, options]
    if (args.size() < 1 || args.size() > 2) {
        throw std::runtime_error("curlGet: expects url and optional options object");
    }
    std::string         url        = Symbols::Value::to_string(args[0]);
    // parse options
    struct curl_slist * headers    = nullptr;
    long                timeoutSec = 0;
    bool                follow     = false;
    if (args.size() == 2) {
        using namespace Symbols;
        if (args[1].getType() != Variables::Type::OBJECT) {
            throw std::runtime_error("curlGet: options must be object");
        }
        const auto & obj = std::get<Value::ObjectMap>(args[1].get());
        for (const auto & kv : obj) {
            const std::string & key = kv.first;
            const Value &       v   = kv.second;
            if (key == "timeout") {
                using namespace Variables;
                switch (v.getType()) {
                    case Type::INTEGER:
                        timeoutSec = v.get<int>();
                        break;
                    case Type::DOUBLE:
                        timeoutSec = static_cast<long>(v.get<double>());
                        break;
                    case Type::FLOAT:
                        timeoutSec = static_cast<long>(v.get<float>());
                        break;
                    default:
                        throw std::runtime_error("curlGet: timeout must be number");
                }
            } else if (key == "follow_redirects") {
                if (v.getType() != Symbols::Variables::Type::BOOLEAN) {
                    throw std::runtime_error("curlGet: follow_redirects must be boolean");
                }
                follow = v.get<bool>();
            } else if (key == "headers") {
                if (v.getType() != Symbols::Variables::Type::OBJECT) {
                    throw std::runtime_error("curlGet: headers must be object");
                }
                const auto & hobj = std::get<Value::ObjectMap>(v.get());
                for (const auto & hk : hobj) {
                    if (hk.second.getType() != Symbols::Variables::Type::STRING) {
                        throw std::runtime_error("curlGet: header values must be string");
                    }
                    std::string line = hk.first + ": " + hk.second.get<std::string>();
                    headers          = curl_slist_append(headers, line.c_str());
                }
            } else {
                throw std::runtime_error("curlGet: unknown option '" + key + "'");
            }
        }
    }
    // initialize handle
    CURL * curl = curl_easy_init();
    if (!curl) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        throw std::runtime_error("curl: failed to initialize");
    }
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (timeoutSec > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    }
    if (follow) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (res != CURLE_OK) {
        std::string error = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl: request failed: " + error);
    }
    curl_easy_cleanup(curl);
    return Symbols::Value(response);
}
