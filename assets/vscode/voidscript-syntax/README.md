# VoidScript Syntax Highlighting

This Visual Studio Code extension provides syntax highlighting for the [VoidScript](https://github.com/fszontagh/voidscript) scripting language.

## Features

- Syntax highlighting for VoidScript source files with extensions `.vs` and `.voidscript`.

## Requirements

- Visual Studio Code version 1.58.0 or higher or compatible VSCodium

## Installation

### From Visual Studio Marketplace

1. Open the Extensions view (`Ctrl+Shift+X` / `Cmd+Shift+X` on macOS).
2. Search for **VoidScript Syntax**.
3. Click **Install**.

### From VSIX Package

1. Download the `voidscript-syntax-<version>.vsix` file from the [Releases](https://github.com/fszontagh/voidscript/releases) page.
2. In VS Code, open the Command Palette (`Ctrl+Shift+P`), then select **Extensions: Install from VSIX...**.
3. Choose the downloaded `.vsix` file.

## Development & Building

To build and package the extension from source:

```bash
git clone https://github.com/fszontagh/voidscript.git
cd voidscript/assets/vscode/voidscript-syntax
npm install
npm run build
```

After building, a `.vsix` package (e.g., `voidscript-syntax-1.0.0.vsix`) will be generated in the current directory.

To install the extension from file: `code --install-extension voidscript-syntax-1.0.0.vsix`

## Usage

Open a VoidScript file (extension `.vs` or `.voidscript`) in VS Code. The syntax highlighting will be applied automatically.

## Contributing

Contributions to the grammar and syntax definitions are welcome!

1. Fork the repository and create a new branch.
2. Make changes under `src/grammar/`.
3. Commit and push your changes.
4. Open a Pull Request.

## License

MIT License. See the [LICENSE](LICENSE) file for details.
