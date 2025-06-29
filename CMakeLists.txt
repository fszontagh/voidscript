cmake_minimum_required(VERSION 3.20)
project(
    voidscript
    LANGUAGES CXX
    VERSION 0.1.1
    DESCRIPTION "A simple scripting language"
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NEED_CLI ON)
set(NEED_EMBEDDED OFF)

option(BUILD_CLI, "Build example commandline interpreter" ${NEED_CLI})
option(BUILD_COMPILER "Build VoidScript compiler" ON)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_FASTCGI "Enable building FastCGI executable" ON)
option(BUILD_TESTS "Build the test cases" OFF)


if (BUILD_CLI)
    set(NEED_CLI ${BUILD_CLI})
    set(NEED_EMBEDDED OFF)
endif()

option(BUILD_EMBEDDED "Build to use embedded scripting" ${NEED_EMBEDDED})


file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory.")
endif()



include(cmake/AppVersion.cmake)

set(IDENTIFIER_COMMENT "#")
set(PARSER_OPEN_TAG "<?void")
set(PARSER_CLOSE_TAG "?>")


if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND BUILD_EMBEDDED)
    status(WARNING "Building embedded version with Debug not recommended!")
endif()


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(DEBUG_BUILD ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()


# PACKAGING PROPERTIES
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/assets/linux/DESCRIPTION.txt")
set(CPACK_PACKAGE_CONTACT "Ferenc Szontágh <szf@fsociety.hu>")
set(CPACK_PACKAGE_VENDOR "fszontagh")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/fszontagh/voidscript")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Voidscript a simple scripting language")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_STRIP_FILES YES)

set(
    CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

set(ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
        COMMAND bash -c "source /etc/lsb-release && echo \"$DISTRIB_CODENAME\""
        OUTPUT_VARIABLE DISTRIB_CODENAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_SYSTEM_NAME ${DISTRIB_CODENAME})
    message(STATUS "Detected DISTRIB_CODENAME: ${DISTRIB_CODENAME}")

    find_program(DPKG dpkg)
    if (DPKG)
        execute_process(COMMAND ${DPKG} --print-architecture OUTPUT_VARIABLE ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()


    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCHITECTURE "amd64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCHITECTURE "i386")
    endif()
endif()

message(STATUS "System architecture: ${ARCHITECTURE}")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/")
endif()

include(GNUInstallDirs)

set(MODULES_FOLDER "${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules" CACHE PATH "OS specific modules folder")

message(STATUS "CMAKE_INSTALL_FULL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR}")

# PACKAGING PROPERTIES END
set(IS_EMBEDDED false)

if (BUILD_EMBEDDED)
    set(IS_EMBEDDED true)
endif()

configure_file("cmake/options.h.in" "include/options.h" @ONLY)
    configure_file("test_scripts/variables.vs" "test_scripts/variables.vs" @ONLY)
    configure_file("test_scripts/expressions.vs" "test_scripts/expressions.vs" @ONLY)




include_directories(${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/src)


# LIBRARY TARGET
add_library(voidscript
            src/Parser/Parser.cpp
            src/Lexer/Lexer.cpp
            src/Lexer/Operators.cpp
            src/Symbols/SymbolContainer.cpp
            src/Symbols/Value.cpp
            src/Symbols/EnumSymbol.cpp
            src/Modules/BuiltIn/ModuleHelperModule.cpp
            src/Interpreter/Interpreter.cpp
            src/Compiler/VoidScriptCompiler.cpp
            src/Compiler/CompilerBackend.cpp
            src/Compiler/CodeGenerator.cpp
            src/Compiler/RuntimeLibrary.cpp
)

install(TARGETS voidscript DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT "lib")
set(CPACK_DEBIAN_LIB_FILE_NAME "libvoidscript_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
set(CPACK_RPM_LIB_FILE_NAME "libvoidscript_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
set(CPACK_DEBIAN_LIB_DESCRIPTION "VoidScript - libraries")
set(CPACK_DEBIAN_LIB_PACKAGE_NAME "libvoidscript")
set(CPACK_DEBIAN_LIB_PACKAGE_SECTION "libs")
set_target_properties(voidscript PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
message(STATUS "CMAKE_SYSTEM: ${CMAKE_SYSTEM}")

set_target_properties(voidscript PROPERTIES
    LINKER_LANGUAGE CXX
    LIBRARY_OUTPUT_NAME voidscript
)


# EXECUTABLE TARGET
if (NEED_CLI)
    add_executable(voidscript-cli cli/main.cpp)
    # Mark this build as CLI variant
    target_compile_definitions(voidscript-cli PRIVATE CLI)
    # Also define CLI for the library when building for CLI
    target_compile_definitions(voidscript PRIVATE CLI)
    target_link_libraries(voidscript-cli voidscript)
    install(TARGETS voidscript-cli DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "bin")
    set_target_properties(voidscript-cli PROPERTIES
        LINKER_LANGUAGE CXX
        OUTPUT_NAME voidscript)

    set(CPACK_PACKAGE_EXECUTABLES voidscript-cli "Simple Shell")

    set(CPACK_DEBIAN_BIN_FILE_NAME "${CMAKE_PROJECT_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_BIN_PACKAGE_NAME "voidscript")
    set(CPACK_DEBIAN_BIN_PACKAGE_SECTION "interpreters")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${CMAKE_BINARY_DIR})
    set(CPACK_DEBIAN_BIN_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")

    set(CPACK_RPM_BIN_FILE_NAME "${CMAKE_PROJECT_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    set(CPACK_RPM_BIN_PACKAGE_NAME "voidscript")
    set(CPACK_RPM_BIN_PACKAGE_REQUIRES "libvoidscript = ${CMAKE_PROJECT_VERSION}")
endif()

# COMPILER TARGET
if (BUILD_COMPILER)
    add_subdirectory(compiler)
    
    set(CPACK_DEBIAN_COMPILER_FILE_NAME "${CMAKE_PROJECT_NAME}-compiler_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_COMPILER_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-compiler")
    set(CPACK_DEBIAN_COMPILER_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_COMPILER_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")
    
    set(CPACK_RPM_COMPILER_FILE_NAME "${CMAKE_PROJECT_NAME}-compiler_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    set(CPACK_RPM_COMPILER_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-compiler")
    set(CPACK_RPM_COMPILER_PACKAGE_GROUP "Development/Tools")
    set(CPACK_RPM_COMPILER_PACKAGE_REQUIRES "libvoidscript = ${CMAKE_PROJECT_VERSION}")
    set(CPACK_COMPONENT_COMPILER_DESCRIPTION "VoidScript - native code compiler")
endif()

if (BUILD_FASTCGI)
    add_subdirectory(fastcgi)

    set(CPACK_DEBIAN_FCGI_FILE_NAME "${CMAKE_PROJECT_NAME}-fcgi_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_FCGI_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-fcgi")
    set(CPACK_DEBIAN_FCGI_PACKAGE_SECTION "web")
    set(CPACK_DEBIAN_FCGI_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")

    set(CPACK_RPM_FCGI_FILE_NAME "${CMAKE_PROJECT_NAME}-fcgi_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    set(CPACK_RPM_FCGI_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-fcgi")
    set(CPACK_RPM_FCGI_PACKAGE_GROUP "Applications/Web")
    set(CPACK_RPM_FCGI_PACKAGE_REQUIRES "libvoidscript = ${CMAKE_PROJECT_VERSION}")
    set(CPACK_COMPONENT_FCGI_DESCRIPTION "VoidScript - fastCGI implementation")
endif()


## MODULES

macro(add_dynamic_module MODULE_NAME)
    string(TOUPPER "${MODULE_NAME}" UPPER_MODULE_NAME)
    string(TOLOWER "${MODULE_NAME}" LOWER_MODULE_NAME)
    set(opt_name "BUILD_MODULE_${UPPER_MODULE_NAME}")
    option(${opt_name} "Enable ${UPPER_MODULE_NAME} module" ON)

    if(${opt_name})
        include(Modules/${MODULE_NAME}/module.cmake OPTIONAL)
        message(STATUS "Adding module:     ${MODULE_NAME}  ${MODULE_DESCRIPTION}")
        set_property(GLOBAL PROPERTY MODULE_DESCRIPTION_${UPPER_MODULE_NAME} "${MODULE_DESCRIPTION}")

        add_subdirectory(Modules/${MODULE_NAME})

        string(CONCAT debian_var_name "CPACK_DEBIAN_" "MODULES-${UPPER_MODULE_NAME}" "_FILE_NAME")
        string(CONCAT debian_desc_name "CPACK_RPM_" "MODULES-${UPPER_MODULE_NAME}" "_DESCRIPTION")
        string(CONCAT rpm_var_name "CPACK_RPM_" "MODULES-${UPPER_MODULE_NAME}" "_FILE_NAME")
        string(CONCAT rpm_desc_name "CPACK_RPM_" "MODULES-${UPPER_MODULE_NAME}" "_DESCRIPTION")

        set(${debian_var_name} "${CMAKE_PROJECT_NAME}-modules-${LOWER_MODULE_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
        set(${rpm_var_name}    "${CMAKE_PROJECT_NAME}-modules-${LOWER_MODULE_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
        set(${debian_desc_name} "${MODULE_DESCRIPTION}")
        set(${rpm_desc_name} "${MODULE_DESCRIPTION}")

    endif()
endmacro()


file(GLOB AVAILABLE_MODULES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/Modules
    "Modules/*"
)


message(STATUS "APP_GIT_VERSION:       ${APP_GIT_VERSION}")
message(STATUS "APP_VERSION:           ${CMAKE_PROJECT_VERSION}\n")
message(STATUS "BUILD_EMBEDDED:        ${BUILD_EMBEDDED}")
message(STATUS "BUILD_CLI:             ${NEED_CLI}")
message(STATUS "BUILD_COMPILER:        ${BUILD_COMPILER}")
message(STATUS "BUILD_FASTCGI:         ${BUILD_FASTCGI}")
message(STATUS "BUILD_SHARED_LIBS:     ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_TESTS:           ${BUILD_TESTS}\n")
message(STATUS "  PARSER_OPEN_TAG:     '${PARSER_OPEN_TAG}'")
message(STATUS "  PARSER_CLOSE_TAG:    '${PARSER_CLOSE_TAG}'\n")

if (NOT BUILD_EMBEDDED)
    message(STATUS "  MODULES_FOLDER:            ${MODULES_FOLDER}")
    foreach(MODULE IN LISTS AVAILABLE_MODULES)
        add_dynamic_module(${MODULE})
    endforeach(MODULE AVAILABLE_MODULES)
endif()

# CPACK CONFIGURATION
set(CPACK_DEB_COMPONENT_INSTALL ON)


if (BUILD_TESTS)
  enable_testing()
  include(cmake/catch2.cmake)
  include(Catch)


  # Test executable for ModuleHelperModule
  add_executable(module_helper_tests
      tests/ModuleHelperTests.cpp
  )
  target_link_libraries(module_helper_tests PRIVATE voidscript Catch2::Catch2WithMain)
  catch_discover_tests(module_helper_tests)

  # Test executable for all built-in modules
  add_executable(built_in_module_tests
      tests/BuiltInModuleTests.cpp
  )
  target_link_libraries(built_in_module_tests PRIVATE voidscript Catch2::Catch2WithMain)
  catch_discover_tests(built_in_module_tests)



  # Test executable for symbol container
  add_executable(symbol_container_tests
      tests/SymbolContainerTests.cpp
  )
  target_link_libraries(symbol_container_tests PRIVATE voidscript Catch2::Catch2WithMain)
  catch_discover_tests(symbol_container_tests)

  # Ensure voidscript target exists before adding tests that use it
  if(TARGET voidscript)
      add_test(NAME WhileLoopTest
               COMMAND ${CMAKE_BINARY_DIR}/voidscript ${CMAKE_SOURCE_DIR}/test_scripts/while_loop_test.vs)

      add_test(NAME CStyleForLoopTest
               COMMAND ${CMAKE_BINARY_DIR}/voidscript ${CMAKE_SOURCE_DIR}/test_scripts/cstyle_for_loop_test.vs)

      add_test(NAME ObjectTest
               COMMAND ${CMAKE_BINARY_DIR}/voidscript ${CMAKE_SOURCE_DIR}/test_scripts/object.vs)

      add_test(NAME ClassTest
               COMMAND ${CMAKE_BINARY_DIR}/voidscript ${CMAKE_SOURCE_DIR}/test_scripts/class.vs)

      add_test(NAME ArrayTest
               COMMAND ${CMAKE_BINARY_DIR}/voidscript ${CMAKE_SOURCE_DIR}/test_scripts/array.vs)

      # Add more tests here if needed...

  endif()
  
  # Add compiler tests if compiler is enabled
  if(BUILD_COMPILER)
      add_subdirectory(tests/compiler)
  endif()
endif()


include(CPack)
