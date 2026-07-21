cmake_minimum_required(VERSION 3.20)
project(
    voidscript
    LANGUAGES CXX
    VERSION 0.2.0
    DESCRIPTION "A simple scripting language"
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Please select another Build Directory! CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} is the same as CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}. In-source builds are discouraged for this project.")
endif()

set(CMAKE_CXX_STANDARD 20)

# An enum switch that gains a new case must not compile until every switch handles it.
# A `default: throw` arm once hid a missing Operations::Type::ControlFlow case and broke
# the entire `switch` statement at runtime instead of at build time.
add_compile_options(-Wswitch -Werror=switch)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NEED_CLI ON)
set(NEED_EMBEDDED OFF)

option(BUILD_CLI, "Build example commandline interpreter" ${NEED_CLI})
option(BUILD_COMPILER "Build VoidScript compiler" ON)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_FASTCGI "Enable building FastCGI executable" ON)
option(BUILD_MODULE_MONGODB "Enable MongoDB module" ON)
option(BUILD_MODULE_MEMCACHED "Enable Memcached module" ON)
option(BUILD_MODULE_CURL "Enable Curl module" ON)
option(BUILD_MODULE_FORMAT "Enable Format module" ON)
option(BUILD_MODULE_IMAGICK "Enable Imagick module" ON)
option(BUILD_MODULE_XML2 "Enable Xml2 module" ON)
option(BUILD_MODULE_MARIADB "Enable MariaDB module" ON)
option(BUILD_MODULE_HASH "Enable Hash module (OpenSSL)" ON)
option(BUILD_MODULE_ARCHIVE "Enable Archive module (libarchive)" ON)
option(BUILD_TESTS "Build the test cases" OFF)


if (BUILD_CLI)
    set(NEED_CLI ${BUILD_CLI})
    set(NEED_EMBEDDED OFF)
endif()

option(BUILD_EMBEDDED "Build to use embedded scripting" ${NEED_EMBEDDED})


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
            src/Modules/BuiltIn/JsonConverters.cpp
            src/Modules/BuiltIn/JsonModule.cpp
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

# Handle MongoDB module like other modules
list(REMOVE_ITEM AVAILABLE_MODULES MongoDb)
if(BUILD_MODULE_MONGODB)
    add_dynamic_module(MongoDb)
endif()

# Handle Memcached module like other modules
list(REMOVE_ITEM AVAILABLE_MODULES Memcached)
if(BUILD_MODULE_MEMCACHED)
    add_dynamic_module(Memcached)
endif()

# Handle MariaDB module like other modules
list(REMOVE_ITEM AVAILABLE_MODULES MariaDb)
if(BUILD_MODULE_MARIADB)
    add_dynamic_module(MariaDb)
endif()


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

      # Every script in test_scripts/ must run clean and unattended. Scripts that
      # need an interactive TTY or an input file now detect its absence and skip,
      # rather than failing, so this is a meaningful all-green target. The absolute
      # path matters: `include` resolves against the parent of the path as given.
      file(GLOB VS_SUITE_SCRIPTS "${CMAKE_SOURCE_DIR}/test_scripts/*.vs")
      foreach(VS_SCRIPT ${VS_SUITE_SCRIPTS})
          get_filename_component(VS_SCRIPT_NAME ${VS_SCRIPT} NAME_WE)
          # stdin must be closed: ctest otherwise hands the tests the terminal and the
          # interactive scripts block forever waiting for a line that never comes.
          add_test(NAME Script.${VS_SCRIPT_NAME}
                   COMMAND sh -c "'${CMAKE_BINARY_DIR}/voidscript' '${VS_SCRIPT}' < /dev/null")
          set_tests_properties(Script.${VS_SCRIPT_NAME} PROPERTIES TIMEOUT 30)
      endforeach()


      # --- Regression tests (see BUGS.md) ---

      # Bug #1: a declaration missing the $ sigil must be a prompt syntax error,
      # not an infinite parser loop. The TIMEOUT is the actual assertion here.
      add_test(NAME RegressionMissingSigilSyntaxError
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/missing_sigil_syntax_error.vs)
      set_tests_properties(RegressionMissingSigilSyntaxError PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "Syntax ERROR")

      # Bug #2: return from a function called in statement position must not
      # abort the process with an uncaught ReturnException.
      add_test(NAME RegressionBareReturn
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/bare_return.vs)
      set_tests_properties(RegressionBareReturn PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED;terminate called")

      # Bug #3: switch must be dispatched by the interpreter, not fall through
      # to "Unknown operation type".
      add_test(NAME RegressionSwitchStatement
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/switch_statement.vs)
      set_tests_properties(RegressionSwitchStatement PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED;Unknown operation type")

      # Bug #4: '[' must bind as a postfix accessor, so indexing composes with
      # member access and works as an assignment target.
      add_test(NAME RegressionIndexOperator
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/index_operator.vs)
      set_tests_properties(RegressionIndexOperator PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "20\n99\n30\n77\n3\n30\ndone\n3\n4\n8\nappend-ok")

      # Bug #11: a top-level return must end the script, not escape main and abort
      # the process via std::terminate.
      add_test(NAME RegressionTopLevelReturn
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/toplevel_return.vs)
      set_tests_properties(RegressionTopLevelReturn PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED;terminate called")

      # Bug #10: numeric initialisers adopt the declared numeric type, since every
      # float literal lexes as double.
      add_test(NAME RegressionNumericConversion
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/numeric_conversion.vs)
      set_tests_properties(RegressionNumericConversion PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "done")

      # Bug #6: sized array declarations, `int $codes[3];`
      add_test(NAME RegressionSizedArrayDecl
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/sized_array_decl.vs)
      set_tests_properties(RegressionSizedArrayDecl PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "0\n200\n404\nsecond\n0.000000\ndone")

      # Bug #5: bitwise operators, and the C-like precedence ladder they need.
      add_test(NAME RegressionBitwiseOperators
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/bitwise_operators.vs)
      set_tests_properties(RegressionBitwiseOperators PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "8\n14\n6\n48\n3\n-13\n7\n2\ntrue\n7\ntrue\nfalse\ntrue\ndone")

      # Bugs #16/#17: switch must dispatch on strings, and a switch ending a function
      # body must parse (function bodies are re-parsed from a slice with no EOF token).
      add_test(NAME RegressionSwitchStringAndReturn
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/switch_string_and_return.vs)
      # Bug #24: `new` must invoke a native (module-supplied) constructor, and the
      # DateTime class must store its timestamp where its own methods look for it.
      add_test(NAME RegressionDateTimeObject
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/datetime_object.vs)
      set_tests_properties(RegressionDateTimeObject PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ndone")

      # Imagick composite, plus the crop/blur/rotate/flip revival and the per-instance
      # handle fix (two images from the same file must be independent).
      add_test(NAME RegressionImagickComposite
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/imagick_composite.vs)
      set_tests_properties(RegressionImagickComposite PROPERTIES
               TIMEOUT 20
               PASS_REGULAR_EXPRESSION "32,32,128\n8x8\n80,80,128\n255,0,0\ntrue\ndone")

      # Integer division/modulo by zero must throw, not SIGFPE the process.
      add_test(NAME RegressionDivisionByZero
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/division_by_zero.vs)
      set_tests_properties(RegressionDivisionByZero PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED"
               PASS_REGULAR_EXPRESSION "div caught\nmod caught\nvar div caught\n5\n1\n3.500000\ndone")

      # Method-call depth counter must measure nesting, not cumulative calls.
      add_test(NAME RegressionMethodCallDepth
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/method_call_depth.vs)
      set_tests_properties(RegressionMethodCallDepth PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "500\ndone")

      # Task #9: Imagick per-pixel access, and optional native-method parameters.
      add_test(NAME RegressionImagickPixels
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/imagick_pixels.vs)
      set_tests_properties(RegressionImagickPixels PROPERTIES
               TIMEOUT 20
               FAIL_REGULAR_EXPRESSION "NOT REACHED"
               PASS_REGULAR_EXPRESSION "16x16\n32,48,128\n255,0,0\n10,20,30,128\n255,0,0\nout-of-range rejected\ndone")

      # Task #7: the String module beyond length/replace/substr.
      add_test(NAME RegressionStringFunctions
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/string_functions.vs)
      set_tests_properties(RegressionStringFunctions PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "MIXED\nmixed\npadded\\|\n6\n-1\ntrue\nfalse\ntrue\ntrue\nababab\ncba\n3\nb\na-b-c\n1\nabc")

      # Task #6: base64 and sleep, which were absent from the whole tree.
      add_test(NAME RegressionBase64Sleep
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/base64_sleep.vs)
      set_tests_properties(RegressionBase64Sleep PROPERTIES
               TIMEOUT 20
               PASS_REGULAR_EXPRESSION "SGVsbG8sIFZvaWRTY3JpcHQh\nHello, VoidScript!\na\nab\nabc\n\\|\ntrue\ndone")

      # Task #5: `continue`, and a clear diagnostic for break/continue outside a loop.
      add_test(NAME RegressionBreakContinue
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/break_continue.vs)
      set_tests_properties(RegressionBreakContinue PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED"
               PASS_REGULAR_EXPRESSION "a0\na1\na3\na4\nb1\nb2\nb4\nb5\ncx\ncy\nd0\nd2\ne00\ne02\ne10\ne12\nf0\nf2\ndone")

      # Task #8: the OOP CurlClient and the unregistered curl free functions.
      # Uses a file:// URL, so no network or listening port is involved.
      add_test(NAME RegressionCurlClient
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/curl_client.vs)
      set_tests_properties(RegressionCurlClient PROPERTIES
               TIMEOUT 20
               PASS_REGULAR_EXPRESSION "true\ntrue\ntrue\ntrue\n\\{\"ok\":true\\}\n\\{\"which\":\"first\"\\}\n\\{\"which\":\"second\"\\}\ndone")

      # Bug #23: `auto` as a general declaration type, inferring from the initialiser.
      add_test(NAME RegressionAutoType
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/auto_type.vs)
      set_tests_properties(RegressionAutoType PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "42\ntext\n3.500000\ntrue\n43\ntext!\nv\n2\ntrue\n7\nk\ndone")

      # Bug #22: a for loop's induction variable must be scoped to the loop, so the
      # same name can be reused by a later loop.
      add_test(NAME RegressionLoopScope
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/loop_scope.vs)
      set_tests_properties(RegressionLoopScope PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "a0\na1\nb0\nb1\nc00\nc01\nc10\nc11\ndx\ndy\nex\ney\nf0\nf1\ndone\nk=3\nscope-done\ns0\ns2\ns4\nc0\nc3\nincr-done")

      # Bug #21: hex/binary/octal literals, scientific notation, digit separators.
      add_test(NAME RegressionNumericLiterals
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/numeric_literals.vs)
      set_tests_properties(RegressionNumericLiterals PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "255\n65280\n255\n255\n10\n15\n15\n1000.000000\n150.000000\n0.020000\n1000000\n65535\n42\n3.500000\n0\n16711680\n255\ndone")

      # Bug #8: string interpolation in double-quoted literals, with escaping and
      # single quotes as the opt-out.
      add_test(NAME RegressionStringInterpolation
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/string_interpolation.vs)
      set_tests_properties(RegressionStringInterpolation PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "Hello world\nn=42 d=1.500000\nworldworld\n\\[world\\]\nworlds\ncosts \\$5\n100% \\$ ok\nraw \\$name\ngot inner\nworld\ndone")

      # Bug #7: try / catch / throw. The test also pins that catch does NOT swallow
      # break (BreakException derives from std::exception) or return.
      add_test(NAME RegressionTryCatch
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/try_catch.vs)
      set_tests_properties(RegressionTryCatch PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED"
               PASS_REGULAR_EXPRESSION "caught-bare\ndetailed message\n3\ncaught-runtime\ntry-ok\nafter\ninner-caught\nouter-continues\nrethrown\nok\nrecovered:failed\n1\nreturned\ndone")

      # Bug #14: the ternary conditional, including right associativity, precedence,
      # and that only the taken branch is evaluated.
      add_test(NAME RegressionTernary
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/ternary.vs)
      set_tests_properties(RegressionTernary PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "big\nsmall\nover\n5\nB\nyes\npos\n1\ndone")

      # Bug #9: string + number concatenation. Note the test also pins that numeric
      # '+' stays arithmetic, so this cannot regress into JS-style "2"+3 == "23".
      add_test(NAME RegressionStringConcat
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/string_concat.vs)
      set_tests_properties(RegressionStringConcat PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "i=42\n42=i\nd=3.500000\nb=true\na42b\nxy\n5\n3.500000\ndone\nx5\nconcat-done")

      # Bug #19: json_encode and var_dump mishandled falsy values, via the same
      # ValuePtr::operator bool() trap as #18.
      add_test(NAME RegressionFalsyValues
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/falsy_values.vs)
      set_tests_properties(RegressionFalsyValues PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "0\n\"\"\nfalse\n42\n\"hi\"\ntrue")

      # Bug #15: runtime errors must report a real source location, not stack garbage.
      # This script deliberately fails; PASS_REGULAR_EXPRESSION makes the exit code
      # irrelevant and asserts on where the error claims to be.
      add_test(NAME RegressionErrorSourceLocation
               COMMAND ${CMAKE_BINARY_DIR}/voidscript
                       ${CMAKE_SOURCE_DIR}/test_scripts/regression/error_source_location.vs)
      set_tests_properties(RegressionErrorSourceLocation PROPERTIES
               TIMEOUT 10
               PASS_REGULAR_EXPRESSION "at line: 2, column: 7")

      set_tests_properties(RegressionSwitchStringAndReturn PROPERTIES
               TIMEOUT 10
               FAIL_REGULAR_EXPRESSION "NOT REACHED;end of stream"
               PASS_REGULAR_EXPRESSION "deleting\nunknown-ok\na-or-b\none\ntwo\nother\nyes\nno\ntwo-ok\ndone\nzero-ok\nempty-ok\nred-ok\nfalsy-done")

  endif()

  # Add compiler tests if compiler is enabled
  if(BUILD_COMPILER)
      add_subdirectory(tests/compiler)
  endif()
endif()


include(CPack)
