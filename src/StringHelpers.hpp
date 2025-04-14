#ifndef STRINGHELPERS_HPP
#define STRINGHELPERS_HPP
#include <algorithm>
#include <string>

namespace StringHelpers {

static std::string extractSubstring(const std::string & str, const size_t & start, const size_t & end) {
    if (start >= 0 && start < str.length() && end >= start && end < str.length()) {
        return str.substr(start, end - start + 1);
    }
    return "";
}

static void strtolower(std::string & data) {
    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });
}

};  // namespace StringHelpers
#endif  // STRINGHELPERS_HPP
