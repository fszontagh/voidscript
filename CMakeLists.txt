cmake_minimum_required(VERSION 3.16)
project(sonyscript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


option(BUILD_CLI, "Build example commandline intrepeter" ON)
option(BUILD_TESTS, "Build tests" OFF)
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" ON)

set(COMMENT_CHARACTER "#")

message(STATUS "BUILD_CLI: ${BUILD_CLI}")
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "\tCOMMENT_CHARACTER: ${COMMENT_CHARACTER}")

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE is not set, defaulting to Release")
endif()



message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

configure_file("cmake/options.h.in" "include/options.h" @ONLY)
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(src)


file(GLOB_RECURSE SOURCES
    src/main.cpp
    src/SScriptInterpreter.cpp
    src/Lexer.cpp
)

if (BUILD_SHARED_LIBS)
    add_library(${CMAKE_PROJECT_NAME} SHARED)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SOURCES})
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
endif()
if (BUILD_STATIC_LIBS)
    add_library(${CMAKE_PROJECT_NAME}_static STATIC)
    target_sources(${CMAKE_PROJECT_NAME}_static PRIVATE ${SOURCES})
    set_target_properties(${CMAKE_PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
endif()


set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

if (BUILD_CLI)
    add_executable(cli cli/main.cpp)
    add_dependencies(cli ${CMAKE_PROJECT_NAME})
    if (BUILD_SHARED_LIBS)
        target_link_libraries(cli ${CMAKE_PROJECT_NAME})
        else()
        target_link_libraries(cli ${CMAKE_PROJECT_NAME}_static)
    endif()
endif()