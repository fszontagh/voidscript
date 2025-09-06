if (NOT BUILD_MODULE_FORMAT)
    return()
endif()

project(
    modules-format
    VERSION 0.0.1
    DESCRIPTION "A simple fmt module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)


find_package(fmt REQUIRED)


add_library(voidscript-module-format SHARED)
target_sources(voidscript-module-format
    PRIVATE
        src/FormatModule.cpp
        src/PluginInit.cpp
)

target_link_libraries(voidscript-module-format PRIVATE fmt::fmt)

install(TARGETS voidscript-module-format DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-format")