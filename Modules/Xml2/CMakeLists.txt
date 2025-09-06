# Modules/IMAGICK/CMakeLists.txt

if (NOT BUILD_MODULE_XML2)
    return()
endif()

include(module.cmake)

project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "XML parser based on libxml2"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_package(LibXml2 REQUIRED)
add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/XmlModule.cpp
        src/PluginInit.cpp
)

target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE ${LIBXML2_LIBRARIES})

install(TARGETS voidscript-module-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
