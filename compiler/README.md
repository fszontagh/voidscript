# VoidScript Compiler

The VoidScript Compiler is a command-line tool that compiles VoidScript source files to native executables.

## Building

The compiler is built as part of the main VoidScript build system. To build the compiler:

```bash
mkdir -p build
cd build
cmake ..
ninja voidscript-compiler
```

Or build everything including the compiler:

```bash
ninja
```

The compiler executable will be created at `build/compiler/voidscript-compiler`.

## Usage

### Basic Usage

```bash
voidscript-compiler script.vs
```

This compiles `script.vs` to an executable with the same name (without extension).

### Command Line Options

- `--help`: Show help message and available options
- `--version`: Show version information
- `--debug`: Enable debug compilation (includes debug symbols)
- `--optimize`: Enable optimization (O2 level)
- `--output <file>`: Specify output file path (default: input filename without extension)
- `--target <arch>`: Target architecture (x86_64, i386, arm64)
- `--generate-assembly`: Generate assembly file only (don't compile to binary)
- `--keep-intermediate`: Keep intermediate assembly files
- `--compiler <path>`: Specify C compiler to use (default: gcc)
- `--include <dir>`: Add include directory
- `--library-path <dir>`: Add library search path
- `--library <name>`: Link with library

### Examples

```bash
# Basic compilation
voidscript-compiler hello.vs

# Debug build with custom output name
voidscript-compiler --debug --output hello_debug hello.vs

# Optimized build for specific architecture
voidscript-compiler --optimize --target x86_64 --output hello_optimized hello.vs

# Generate assembly only
voidscript-compiler --generate-assembly hello.vs

# Compile with custom includes and libraries
voidscript-compiler --include /usr/local/include --library-path /usr/local/lib --library mylib hello.vs
```

## Integration with CMake

The compiler is integrated into the VoidScript CMake build system:

- **Build Option**: `BUILD_COMPILER` (default: ON)
- **Target Name**: `voidscript-compiler`
- **Dependencies**: Links with the main `voidscript` library
- **Install Component**: `compiler`

To disable the compiler build:

```bash
cmake -DBUILD_COMPILER=OFF ..
```

## Architecture

The compiler CLI integrates with the existing VoidScript components:

- Uses the same **Lexer** and **Parser** as the interpreter
- Integrates with the **VoidScriptCompiler** class from `src/Compiler/`
- Processes operations through the **CompilerBackend**
- Generates native executables via **CodeGenerator** and **RuntimeLibrary**

## Current Status

The compiler CLI interface and CMake integration are complete. The compiler backend implementation is in development and may require additional work for full functionality.

## Files

- `compiler/main.cpp`: Main CLI application
- `compiler/CMakeLists.txt`: CMake configuration for compiler target
- `src/Compiler/VoidScriptCompiler.hpp/.cpp`: Main compiler class
- `src/Compiler/CompilerBackend.hpp/.cpp`: Compiler backend
- `src/Compiler/CodeGenerator.hpp/.cpp`: Code generation
- `src/Compiler/RuntimeLibrary.hpp/.cpp`: Runtime library support