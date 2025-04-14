## voidscript

A lightweight scripting language designed for simplicity and ease of use.

### Features

- Simple and intuitive syntax
- Built-in standard library
- Cross-platform compatibility
- Easy to embed in other applications
- No external dependencies

### Installation

```bash
git clone https://github.com/fszontagh/voidscript.git
cd voidscript
cmake -S . -B build
cmake --build build
```

### Quick Start

```voidscript
# Hello World example
print("Hello,", "", "World!", "\n");

# Variables
int x = 42;
string name = "VoidScript";

// Functions
function $add(int $a, double $b, string $help) {
    int $result = $a + $b;
    print("The sum is: ", $result, "\n");
    print("Help: ", $help, "\n");
    return $result;
}
```

### Documentation

For detailed documentation, please visit the [Wiki](https://github.com/fszontagh/voidscript/wiki).

### Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.