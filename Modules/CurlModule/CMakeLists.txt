project(
    CurlModule
    VERSION 0.0.1
    DESCRIPTION "A simple curl module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(CURL REQUIRED)

add_library(CurlModule SHARED)
target_sources(CurlModule
    PRIVATE
        src/CurlModule.cpp
        src/PluginInit.cpp
)

target_link_libraries(CurlModule PRIVATE curl)