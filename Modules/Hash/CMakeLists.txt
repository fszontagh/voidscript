if (NOT BUILD_MODULE_HASH)
    return()
endif()
include(module.cmake)
project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "Cryptographic hashes via OpenSSL EVP"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(OpenSSL REQUIRED)

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/HashModule.cpp
        src/PluginInit.cpp
)
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE OpenSSL::Crypto)

install(TARGETS voidscript-module-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
