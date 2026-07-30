# Modules/Redis/CMakeLists.txt - a Redis client over raw sockets (no external dependency).
if (NOT BUILD_MODULE_REDIS)
    return()
endif()
include(module.cmake)
project(modules-${MODULE_NAME} VERSION 0.0.1 DESCRIPTION "${MODULE_DESCRIPTION}"
        LANGUAGES CXX HOMEPAGE_URL "https://github.com/fszontagh/voidshell")

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME} PRIVATE src/RedisModule.cpp src/PluginInit.cpp)
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript)
install(TARGETS voidscript-module-${MODULE_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
