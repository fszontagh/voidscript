#ifndef _VOIDUTILS_HPP
#define _VOIDUTILS_HPP

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include <cstring>
#include <string>

namespace utils {

// std::filesystem::exists
inline bool exists(const std::string & path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

// std::filesystem::is_directory
inline bool is_directory(const std::string & path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return S_ISDIR(info.st_mode);
}

// std::filesystem::path...stem..string
inline std::string get_filename_stem(const std::string & path) {
    size_t      slash    = path.find_last_of("/\\");
    std::string filename = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t      dot      = filename.find_last_of('.');
    return (dot == std::string::npos) ? filename : filename.substr(0, dot);
}

// std::filesystem::file_size
inline size_t file_size(const std::string & path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return 0;
    }
    return info.st_size;
}

inline std::string get_parent_directory(const std::string & path) {
    size_t slash_pos = path.find_last_of("/\\");
    if (slash_pos == std::string::npos) {
        return "";
    }
    return path.substr(0, slash_pos);
}

// std::filesystem::create_directories
inline bool create_directories(const std::string & path) {
    if (path.empty()) {
        return false;
    }
    
    // Check if the directory already exists
    if (exists(path) && is_directory(path)) {
        return true;
    }
    
    // Get parent directory
    std::string parent = get_parent_directory(path);
    
    // If parent is not empty and doesn't exist, recursively create it
    if (!parent.empty() && !exists(parent)) {
        if (!create_directories(parent)) {
            return false;
        }
    }
    
    // Create the directory
    return mkdir(path.c_str(), 0755) == 0;
}

// Create a single directory (non-recursive)
inline bool create_directory(const std::string & path) {
    if (exists(path) && is_directory(path)) {
        return true;
    }
    return mkdir(path.c_str(), 0755) == 0;
}

// Remove an empty directory
inline bool remove_directory(const std::string & path) {
    if (!exists(path)) {
        return false;
    }
    if (!is_directory(path)) {
        return false;
    }
    return rmdir(path.c_str()) == 0;
}

// std::filesystem::recursive_directory_iterator
template <typename Callback>
inline void recursive_directory_iterator(const std::string & directory, Callback callback) {
    DIR * dir = opendir(directory.c_str());
    if (!dir) {
        return;
    }

    struct dirent * entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string fullPath = directory + "/" + entry->d_name;
        callback(fullPath);

        if (is_directory(fullPath)) {
            recursive_directory_iterator(fullPath, callback);
        }
    }

    closedir(dir);
}

}  // namespace utils
#endif  // _VOIDUTILS_HPP
