if (NOT BUILD_MODULE_MONGODB)
    return()
endif()

include(module.cmake)
project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "MongoDB support module for VoidScript"
    LANGUAGES CXX
)

# Find MongoDB C++ driver using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
pkg_check_modules(BSONCXX REQUIRED libbsoncxx)

message(STATUS "MONGOCXX_INCLUDE_DIRS: ${MONGOCXX_INCLUDE_DIRS}")
message(STATUS "BSONCXX_INCLUDE_DIRS: ${BSONCXX_INCLUDE_DIRS}")
message(STATUS "MONGOCXX_LIBRARIES: ${MONGOCXX_LIBRARIES}")
message(STATUS "BSONCXX_LIBRARIES: ${BSONCXX_LIBRARIES}")

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/MongoDBModule.cpp
        src/PluginInit.cpp
)
target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${MONGOCXX_INCLUDE_DIRS} ${BSONCXX_INCLUDE_DIRS})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript ${MONGOCXX_LIBRARIES} ${BSONCXX_LIBRARIES})

install(TARGETS voidscript-module-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")