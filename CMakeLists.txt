cmake_minimum_required(VERSION 3.5)
project(
    voidscript
    LANGUAGES CXX
    VERSION 0.0.1
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(NEED_CLI ON)
set(NEED_TESTS OFF)
set(NEED_STATIC_LIBS ON)
set(NEED_SHARED_LIBS ON)

option(BUILD_CLI, "Build example commandline intrepeter" ${NEED_CLI})
option(BUILD_TESTS, "Build tests" ${NEED_TESTS})
option(BUILD_SHARED_LIBS "Build shared library" ${NEED_SHARED_LIBS})
option(BUILD_STATIC_LIBS "Build static library" ${NEED_STATIC_LIBS})

if (BUILD_CLI)
    set(NEED_CLI ${BUILD_CLI})
endif()
if (BUILD_TESTS)
    set(NEED_TESTS ${BUILD_TESTS})
endif()

if (BUILD_SHARED_LIBS)
    set(NEED_SHARED_LIBS ${BUILD_SHARED_LIBS})
endif()

if (BUILD_STATIC_LIBS)
    set(NEED_STATIC_LIBS ${BUILD_STATIC_LIBS})
endif()

set(COMMENT_CHARACTER "#")
set(PARSER_OPEN_TAG "<?void")
set(PARSER_CLOSE_TAG "?>")

message(STATUS "BUILD_CLI:           ${NEED_CLI}")
message(STATUS "BUILD_TESTS:         ${NEED_TESTS}")
message(STATUS "BUILD_SHARED_LIBS:   ${NEED_SHARED_LIBS}")
message(STATUS "BUILD_STATIC_LIBS:   ${NEED_STATIC_LIBS}")
message(STATUS "  COMMENT_CHARACTER: ${COMMENT_CHARACTER}")
message(STATUS "  PARSER_OPEN_TAG:   ${PARSER_OPEN_TAG}")
message(STATUS "  PARSER_CLOSE_TAG:  ${PARSER_CLOSE_TAG}")

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(DEBUG_BUILD ON)
endif()

configure_file("cmake/options.h.in" "include/options.h" @ONLY)
configure_file("test_scripts/test1.vs" "test_scripts/test1.vs" @ONLY)
configure_file("test_scripts/test2.vs" "test_scripts/test2.vs" @ONLY)


include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(src)


file(GLOB_RECURSE SOURCES
    src/main.cpp
    src/ScriptInterpreter.cpp
    src/Lexer.cpp
)

if (NEED_SHARED_LIBS)
    add_library(${CMAKE_PROJECT_NAME} SHARED)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SOURCES})
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
endif()
if (NEED_STATIC_LIBS)
    add_library(${CMAKE_PROJECT_NAME}_static STATIC)
    target_sources(${CMAKE_PROJECT_NAME}_static PRIVATE ${SOURCES})
    set_target_properties(${CMAKE_PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
endif()


set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

if (NEED_CLI)
    add_executable(cli cli/main.cpp)
    add_dependencies(cli ${CMAKE_PROJECT_NAME})
    if (BUILD_SHARED_LIBS)
        target_link_libraries(cli ${CMAKE_PROJECT_NAME})
        else()
        target_link_libraries(cli ${CMAKE_PROJECT_NAME}_static)
    endif()
endif()