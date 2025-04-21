# Find FastCGI stdio header and library
find_path(FCGI_STDIO_INCLUDE NAMES fcgi_stdio.h)
find_library(FCGI_LIBRARY NAMES fcgi)

if (FCGI_STDIO_INCLUDE AND FCGI_LIBRARY)
    add_executable(voidscript-fcgi src/main.cpp)
    # Mark this build as FastCGI variant
    target_compile_definitions(voidscript-fcgi PRIVATE FCGI)
    target_include_directories(voidscript-fcgi PRIVATE ${FCGI_STDIO_INCLUDE})
    target_link_libraries(voidscript-fcgi PRIVATE voidscript ${FCGI_LIBRARY})
    install(TARGETS voidscript-fcgi DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "bin")
else()
    message(FATAL "FastCGI library or header not found; skipping fastcgi target")
endif()
