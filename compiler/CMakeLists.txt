# Compiler CLI executable
add_executable(voidscript-compiler main.cpp)

# Mark this build as COMPILER variant
target_compile_definitions(voidscript-compiler PRIVATE COMPILER)

# Link with the main voidscript library and compiler components
target_link_libraries(voidscript-compiler voidscript)

# Set target properties
set_target_properties(voidscript-compiler PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME voidscript-compiler
)

# Install the compiler executable
install(TARGETS voidscript-compiler DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "compiler")