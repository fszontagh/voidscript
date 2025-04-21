// FastCGI interface for VoidScript
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <fcgi_stdio.h>
#include "options.h"
#include "VoidScript.hpp"
#ifdef FCGI
#include "Modules/BuiltIn/HeaderModule.hpp"
#include <algorithm>
#include <cctype>
#endif

int main(int argc, char *argv[]) {
    // FastCGI loop: handle each request on STDIN/STDOUT
    while (FCGI_Accept() >= 0) {
        // Clear headers from previous request
#ifdef FCGI
        Modules::HeaderModule::clearHeaders();
#endif
        // Determine script filename from environment
        const char *pathTranslated = getenv("PATH_TRANSLATED");
        std::string filename;
        if (pathTranslated && pathTranslated[0] != '\0') {
            filename = pathTranslated;
        } else {
            const char *scriptEnv = getenv("SCRIPT_FILENAME");
            if (scriptEnv && scriptEnv[0] != '\0') {
                filename = scriptEnv;
            } else {
                // Fallback to reading from STDIN
                filename = "-";
            }
        }

        // Parse QUERY_STRING into script arguments
        std::vector<std::string> scriptArgs;
        const char *qs = getenv("QUERY_STRING");
        if (qs && qs[0] != '\0') {
            std::string qsStr(qs);
            size_t pos = 0;
            while (pos < qsStr.size()) {
                size_t amp = qsStr.find('&', pos);
                if (amp == std::string::npos) {
                    scriptArgs.emplace_back(qsStr.substr(pos));
                    break;
                } else {
                    scriptArgs.emplace_back(qsStr.substr(pos, amp - pos));
                    pos = amp + 1;
                }
            }
        }

        // Capture standard output and error into buffers
        std::ostringstream outBuf;
        std::ostringstream errBuf;
        auto *oldOut = std::cout.rdbuf();
        auto *oldErr = std::cerr.rdbuf();
        std::cout.rdbuf(outBuf.rdbuf());
        std::cerr.rdbuf(errBuf.rdbuf());

        // Execute the script (enable template tag parsing)
        // code is processed only between PARSER_OPEN_TAG and PARSER_CLOSE_TAG (defined in options.h)
        VoidScript vs(filename,
                      /*debugLexer=*/false,
                      /*debugParser=*/false,
                      /*debugInterp=*/false,
                      /*debugSymbolTable=*/false,
                      /*enableTags=*/true,
                      /*suppressTagsOutside=*/false,
                      scriptArgs);
        int exitCode = vs.run();

        // Restore original streams
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);

        // Output HTTP headers (from header() calls)
        {
            const auto &hdrs = Modules::HeaderModule::getHeaders();
            bool hasCT = false;
            for (const auto &kv : hdrs) {
                printf("%s: %s\r\n", kv.first.c_str(), kv.second.c_str());
                std::string key = kv.first;
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
                if (key == "content-type") hasCT = true;
            }
            if (!hasCT) {
                printf("Content-Type: text/html\r\n");
            }
            printf("\r\n");
        }

        // Write script output
        const std::string &body = outBuf.str();
        if (!body.empty()) {
            fwrite((void*)body.data(), 1, body.size(), stdout);
        }

        // If errors occurred, include them in response
        std::string errors = errBuf.str();
        if (!errors.empty() || exitCode != 0) {
            printf("<pre>");
            if (!errors.empty()) {
                fwrite((void*)errors.data(), 1, errors.size(), stdout);
            } else {
                std::string codeMsg = "Error code: " + std::to_string(exitCode);
                fwrite((void*)codeMsg.data(), 1, codeMsg.size(), stdout);
            }
            printf("</pre>\n");
        }
        fflush(stdout);
    }
    return 0;
}