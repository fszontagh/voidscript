# Vim Integration for VoidScript

This folder contains Vim configuration files to enable filetype detection, syntax highlighting, and indentation for VoidScript (`*.vs`) files.

Directory structure:
- `ftdetect/voidscript.vim` — filetype detection for `*.vs` files
- `syntax/voidscript.vim` — syntax highlighting rules
- `indent/voidscript.vim` — indentation rules

## Installation

Follow these steps to install the Vim settings from this directory.

### 1. Change to this directory
```bash
cd path/to/voidscript/assets/vim
```

### 2. Manual Installation (Vim)
1. Create the necessary directories in your Vim runtime path:
   ```bash
   mkdir -p ~/.vim/ftdetect ~/.vim/syntax ~/.vim/indent
   ```
2. Copy the configuration files:
   ```bash
   cp ftdetect/voidscript.vim ~/.vim/ftdetect/
   cp syntax/voidscript.vim ~/.vim/syntax/
   cp indent/voidscript.vim ~/.vim/indent/
   ```
3. Restart Vim. When you open a `.vs` file, VoidScript filetype will be detected, syntax highlighting will apply, and indentation rules will work.

### 3. Vim 8+ Native Packages
Alternatively, you can use Vim’s native package feature:
```bash
mkdir -p ~/.vim/pack/voidscript/start
cp -r . ~/.vim/pack/voidscript/start/voidscript
```
Restart Vim.

### 4. NeoVim
NeoVim uses a different configuration directory:
```bash
# For Neovim runtime
mkdir -p ~/.config/nvim/ftdetect ~/.config/nvim/syntax ~/.config/nvim/indent
cp ftdetect/voidscript.vim ~/.config/nvim/ftdetect/
cp syntax/voidscript.vim ~/.config/nvim/syntax/
cp indent/voidscript.vim ~/.config/nvim/indent/

# Or using Neovim packages
mkdir -p ~/.local/share/nvim/site/pack/voidscript/start
cp -r . ~/.local/share/nvim/site/pack/voidscript/start/voidscript
```
Restart Neovim.

## Testing

Open any VoidScript file:
```bash
vim example.vs
```
You should see filetype set to `voidscript`, with proper syntax highlighting and indentation.