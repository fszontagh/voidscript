if (NOT BUILD_MODULE_CURL)
    return()
endif()

project(
    modules-curl
    VERSION 0.0.1
    DESCRIPTION "A simple curl module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(CURL REQUIRED)

add_library(modules-curl SHARED)
target_sources(modules-curl
    PRIVATE
        src/CurlModule.cpp
        src/PluginInit.cpp
)

target_link_libraries(modules-curl PRIVATE curl)

install(TARGETS modules-curl DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-curl")