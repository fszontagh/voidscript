# VoidScript

VoidScript is a lightweight, embeddable scripting language designed for simplicity, extensibility, and ease of integration. It provides both a command-line interpreter and a FastCGI-based template engine.

## Features
- Simple, dynamically-typed C-like syntax with object-oriented features
- Command-line interpreter (`voidscript`)
- FastCGI runner (`voidscript-fcgi`) for web templates
- Template parsing: embed `<?void ... ?>` tags inside HTML
- #### Built-in standard library modules:
  - Print: `print()`, `printnl()`, `error()`, `throw_error()`
  - [String utilities](https://github.com/fszontagh/voidscript/blob/main/docs/StringModule.md) (`string_length()`, `string_substr()`, etc.)
  - [Array utilities](https://github.com/fszontagh/voidscript/blob/main/docs/ArrayModule.md) (`sizeof()`, iteration helpers)
  - [File I/O](https://github.com/fszontagh/voidscript/blob/main/docs/FileModule.md) (`file_get_contents()`, `file_put_contents()` etc.)
  - [JSON encode/decode](https://github.com/fszontagh/voidscript/blob/main/docs/JsonModule.md) (`json_encode()`, `json_decode()`)
  - [Variable helpers](https://github.com/fszontagh/voidscript/blob/main/docs/VariableHelpersModule.md) (`typeof()` etc.)
  - [Module helpers](https://github.com/fszontagh/voidscript/blob/main/docs/ModuleHelperModule.md) (`module_list()`, `module_exists()`, `module_info()`)
- HTTP header management (FastCGI only): `header()`
- Dynamic Plugin module support (e.g., [CurlModule](https://github.com/fszontagh/voidscript/tree/main/Modules/CurlModule) for HTTP requests)
- Embeddable library (`libvoidscript`)
- Zero runtime dependencies (except for the optional modules)
- Syntax highlighter and formatter for [vscode / codium](https://github.com/fszontagh/voidscript/blob/main/assets/vscode/voidscript-syntax/README.md) and [vim](https://github.com/fszontagh/voidscript/blob/main/assets/vim/README.md)

## Getting Started

### Prerequisites
- CMake 3.20 or later
- C++20-compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- (Optional) `libfcgi-dev` for FastCGI support
- (Optional) `spawn-fcgi` for Nginx integration
- (Optional) `libcurl4-openssl-dev`| `libcurl4-gnutls-dev` for HTTP requests

### Building
```bash
git clone https://github.com/fszontagh/voidscript.git
cd voidscript
mkdir build && cd build
cmake .. [-DBUILD_FASTCGI=ON] [-DBUILD_MODULE_CURL=ON]
cmake --build . -- -j$(nproc)
sudo cmake --install .
```

- `BUILD_FASTCGI=ON` enables `voidscript-fcgi`.
- `BUILD_MODULE_CURL=ON` builds the CurlModule.

### Installation

By default, binaries install into `/usr/local/bin` and the library into your system library directory. Adjust `CMAKE_INSTALL_PREFIX` as needed.

## Usage

### CLI Interpreter
```bash
voidscript [options] [script.vs] [-- args...]
```
Options:
- `--help`               Show help message
- `--version`            Show version info
- `--debug[=component]`  Enable debug output (`lexer`, `parser`, `interpreter`, `symboltable`)
- `--enable-tags`        Only execute code inside `<?void ?>` tags
- `--suppress-tags-outside`  Hide content outside tags
- `script.vs`            Script file (defaults to stdin)
- `-- args...`           Arguments passed to script (`$argc`, `$argv`)

### FastCGI Runner
Configure Apache or Nginx as documented in `fastcgi/docs/README.md` to serve `.vs` templates. Example template:
```html
<html>
<body>
  <?void
    header("Content-Type", "application/json");
    print(json_encode({"status": "ok"}));
  ?>
</body>
</html>
```

## Language Syntax

### Classes and Object-Oriented Programming

VoidScript supports classes with access control modifiers. **Important**: Class properties and methods must be accessed using `$this->` syntax within class methods.

#### Basic Class Example
```voidscript
class Person {
    private:
    string $name = "Unknown";
    int $age = 0;
    
    public:
    // Constructor
    function construct(string $name, int $age) {
        $this->name = $name;  // Correct: Use $this->
        $this->age = $age;
    }
    
    // Getter methods
    function getName() string {
        return $this->name;   // Correct: Use $this->
    }
    
    function getAge() int {
        return $this->age;    // Correct: Use $this->
    }
    
    // Method that modifies properties
    function setAge(int $newAge) {
        $this->age = $newAge; // Correct: Use $this->
    }
    
    function isAdult() bool {
        return $this->age >= 18; // Correct: Use $this->
    }
}

// Usage
Person $person = new Person("John Doe", 25);
printnl("Name: ", $person->getName());
printnl("Age: ", $person->getAge());
printnl("Is adult: ", $person->isAdult());
```

#### Important Syntax Rules

- **✅ Correct**: Always use `$this->property` and `$this->method()` within class methods
- **❌ Incorrect**: Using bare `this->property` or `this->method()` will produce error messages
- **Access Control**: Classes support `private:` and `public:` sections
- **Constructor**: Optional `construct()` method for initialization

#### Migration from Previous Versions

If you have existing VoidScript code using bare `this->` syntax, update it to use `$this->`:

**Before (Old Syntax):**
```voidscript
function getName() string {
    return this->name;    // ❌ Will cause error
}
```

**After (New Syntax):**
```voidscript
function getName() string {
    return $this->name;   // ✅ Correct
}
```

## Embedding as a Library
Include `libvoidscript` in your CMake project:
```cmake
find_package(voidscript REQUIRED)
target_link_libraries(your_app PRIVATE voidscript)
```

## Contributing
1. Fork the repository
2. Create a branch (`git checkout -b feature/foo`)
3. Make changes & tests
4. Commit & push
5. Open a Pull Request

Please follow established coding conventions when contributing.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
