# Modules/Compress/CMakeLists.txt - gzencode/gzdecode via zlib.
if (NOT BUILD_MODULE_COMPRESS)
    return()
endif()
include(module.cmake)
project(modules-${MODULE_NAME} VERSION 0.0.1 DESCRIPTION "${MODULE_DESCRIPTION}"
        LANGUAGES CXX HOMEPAGE_URL "https://github.com/fszontagh/voidshell")

find_package(ZLIB QUIET)
if (NOT ZLIB_FOUND)
    message(WARNING "Compress module: zlib not found - skipping module")
    return()
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME} PRIVATE src/CompressModule.cpp src/PluginInit.cpp)
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript ZLIB::ZLIB)
install(TARGETS voidscript-module-${MODULE_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
