# Modules/IMAGICK/CMakeLists.txt

if (NOT BUILD_MODULE_XML2)
    return()
endif()

include(module.cmake)

project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "${MODULE_DESCRIPTION}"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(LibXml2 REQUIRED)
add_library(modules-${MODULE_NAME} SHARED)
target_sources(modules-${MODULE_NAME}
    PRIVATE
        src/XmlModule.cpp
        src/PluginInit.cpp
)

target_include_directories(modules-${MODULE_NAME} PRIVATE ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(modules-${MODULE_NAME} PRIVATE ${LIBXML2_LIBRARIES})

install(TARGETS modules-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
