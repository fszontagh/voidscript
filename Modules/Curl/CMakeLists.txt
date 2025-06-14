if (NOT BUILD_MODULE_CURL)
    return()
endif()
include(module.cmake)
project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "Network transfer library"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(CURL REQUIRED)

add_library(modules-${MODULE_NAME} SHARED)
target_sources(modules-${MODULE_NAME}
    PRIVATE
        src/CurlModule.cpp
        src/PluginInit.cpp
)

target_link_libraries(modules-${MODULE_NAME} PRIVATE curl)

install(TARGETS modules-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")