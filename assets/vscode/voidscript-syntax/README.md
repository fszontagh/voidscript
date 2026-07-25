# VoidScript for VS Code

Language support for the [VoidScript](https://github.com/fszontagh/voidscript) scripting
language: syntax highlighting, snippets, completion, hovers and a formatter for `.vs` and
`.voidscript` files.

## Features

- **Syntax highlighting** covering the full language surface: `$`-sigil variables,
  keywords (`enum`, `include`, `auto`, `try`/`catch`, `switch`/`case`, ...), all numeric
  bases (`0xFF`, `0b1010`, `0o17`, `1e3`, `1_000`), and double-quoted string interpolation
  (`"Hello $name"`, `${obj->field}`) alongside single-quoted literal strings.
- **~150 built-in functions** highlighted and documented (extracted from the interpreter),
  from `printnl`/`format` to `process_run`, `curlGet`, `json_encode`, the `string_*`
  family and more.
- **Snippets** for the common constructs: `fn`, `class`, `for`, `foreach`, `foreachkv`,
  `if`, `ifelse`, `switch`, `trycatch`, `enum`, `obj`, `printnl`, `include`.
- **Completion & hover**: keyword, type and built-in-function completion, with signatures
  and one-line docs on hover.
- **Formatter**: brace-aware re-indentation that ignores braces inside strings and
  comments and handles `} else {` / `} catch {` correctly. Run with **Format Document**.

Comments are `//` and `#` (both line comments). VoidScript has no block-comment syntax,
so `/* ... */` is **not** supported by the language.

## Requirements

- Visual Studio Code (or VSCodium) version 1.61.0 or higher.

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

`npm run build` compiles `src/extension.ts` to `out/` and produces a `.vsix`
(e.g. `voidscript-1.1.0.vsix`) in the current directory. Use `npm run compile` (or
`npm run watch`) for just the TypeScript build.

To install the extension from file: `code --install-extension voidscript-1.1.0.vsix`

## Usage

Open a VoidScript file (extension `.vs` or `.voidscript`) in VS Code. Highlighting,
snippets, completion and hovers apply automatically.

### Formatting

Format a document from the Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`) via **Format
Document**, or with `Shift+Alt+F`. The formatter re-indents by block depth and leaves
braces inside strings and comments untouched.

## Contributing

Contributions are welcome. The language data lives in a few places, all verified against
the interpreter build:

1. Highlighting: `src/grammar/voidscript.tmLanguage.json` and
   `src/grammar/language-configuration.json`.
2. Snippets: `src/snippets/voidscript.json`.
3. Completion, hover and the formatter: `src/extension.ts`.

Fork, branch, make changes, and open a Pull Request.

## License

MIT License. See the [LICENSE](LICENSE) file for details.
