// CurlModule implementation: HTTP GET and POST using libcurl
#include "CurlModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <string>
#include <vector>

// Callback for libcurl to write received data into a std::string
static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    buffer->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// Register module functions
void Modules::CurlModule::registerModule() {
    auto& mgr = Modules::ModuleManager::instance();
    // Register HTTP GET: curlGet(url)
    mgr.registerFunction("curlGet", [this](const std::vector<Symbols::Value>& args) -> Symbols::Value {
        return this->curlGet(args);
    });
    // Register HTTP POST: curlPost(url, data)
    mgr.registerFunction("curlPost", [this](const std::vector<Symbols::Value>& args) -> Symbols::Value {
        return this->curlPost(args);
    });
}


Symbols::Value Modules::CurlModule::curlPost(const std::vector<Symbols::Value>& args) {
    if (args.size() != 2) {
        throw std::runtime_error("curlPost: missing URL and data arguments");
    }
    std::string url  = Symbols::Value::to_string(args[0]);
    std::string data = Symbols::Value::to_string(args[1]);

    CURL * curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl: failed to initialize");
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string error = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl: request failed: " + error);
    }

    curl_easy_cleanup(curl);
    return Symbols::Value(response);
}

Symbols::Value Modules::CurlModule::curlGet(const std::vector<Symbols::Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("curlGet: missing URL argument");
    }

    std::string url = Symbols::Value::to_string(args[0]);

    CURL * curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl: failed to initialize");
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string error = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl: request failed: " + error);
    }

    curl_easy_cleanup(curl);
    return Symbols::Value(response);
}
