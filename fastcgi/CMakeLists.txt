# Find FastCGI stdio header and library
find_path(FCGI_STDIO_INCLUDE NAMES fcgi_stdio.h)
find_library(FCGI_LIBRARY NAMES fcgi)

if (FCGI_STDIO_INCLUDE AND FCGI_LIBRARY)
    set(FCGI_PATH "${CMAKE_INSTALL_FULL_LIBEXECDIR}/voidscript-fcgi")

    configure_file(assets/voidscript_fcgi.conf.in ${CMAKE_CURRENT_BINARY_DIR}/assets/voidscript_fcgi.conf @ONLY)

    add_executable(voidscript-fcgi src/main.cpp)
    target_compile_definitions(voidscript-fcgi PRIVATE FCGI)
    target_include_directories(voidscript-fcgi PRIVATE ${FCGI_STDIO_INCLUDE})
    target_link_libraries(voidscript-fcgi PRIVATE voidscript ${FCGI_LIBRARY})
    install(TARGETS voidscript-fcgi DESTINATION ${CMAKE_INSTALL_LIBEXECDIR} COMPONENT "fcgi")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/assets/voidscript_fcgi.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/apache2 COMPONENT "fcgi")
else()
    message(FATAL "FastCGI library or header not found")
endif()