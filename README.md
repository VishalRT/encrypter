# TODO
- handle closeHandle on abrupt close of exe (ctrl + c?)
- user mr bjarnes guidelines from github to enforce best practices on identifiers - DONE
- implement cmake --fresh option, CMAKE_GENERATOR env variable as flag to select build generator(ninja or make)
- Make CLI first then lets move to the service logic part.- DONE
- Service implementation, we should be able to observe file changes from service
- Installation and initiazion of service in windows
- Implementaiton in other OS, Linux and MacOScls
- stop at cmake if there's any issue with it during build
- capture delta of file
- encrypt file to specific destination
- decrypt file and open using windows/default app
- When to encrypt and decrypt to make the file available to user ?
- Should we have some opening point to the file. i.e when file is opened we decrypt and open the txt file ? 
- Removed verbose log to build system, make it optional later
- Remove CXX and LDFLags and check if openssl is found ? 
- GUI - Maybe last when most of the things are completed 

----------------------------------------------------------------------------------------------------------------------------------------
# C++ CORE GUIDELINES ENFORCEMENT

## Overview
This project enforces C++ Core Guidelines naming conventions and layout rules using clangd for real-time VS Code integration.
https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-philosophy

## Environment
- **Clang version**: 20.1.1
- **VS Code Extension**: clangd with clang-tidy integration
- **Configuration**: `.clang-tidy` file for naming rules

## Naming Conventions (C++ Core Guidelines NL Section)
- **Functions**: `snake_case` (NL.10)
- **Variables**: `snake_case` (NL.10) 
- **Constants**: `UPPER_CASE` (NL.9 spirit)
- **Types**: `PascalCase` (conventional)
- **Parameters**: `snake_case` (NL.10)
- **Private members**: `snake_case_` (conventional)

## Layout Guidelines (C++ Core Guidelines NL Section)
- **NL.17**: K&R-derived layout
- **NL.18**: C++-style declarator layout
- **NL.20**: Single statements per line
- **NL.4**: Consistent indentation

## VS Code Setup
1. Install clangd extension from VS Code Marketplace
2. Create `settings.json` in workspace root (already created)
3. Real-time naming violations will appear as squiggles and in Problems panel

## Build-time Enforcement
- `.clang-tidy` file configured for naming rules
- **VS Code Intellisense**: VS Code with clangd extension

**Conditional Build Process:**
1. **CMake Configuration**: `cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -S . -B build`
   - Adds `-DENABLE_CLANG_TIDY_CHECKS=ON` only when `--clang-tidy-check` flag is passed
2. **clang-tidy Checks** (only when requested): `cmake --build build --target clang-tidy-check`

**Usage Examples:**
```bash
# Build with naming checks
python build.py --build-action build --clang-tidy-check

# Build without naming checks (default)
python build.py --build-action build
``` 



----------------------------------------------------------------------------------------------------------------------------------------
# PREREQUISTE
- **Clang 20.1.1** (C++23 compiler)
- **OpenSSL 3.4.1** (Library: OpenSSL 3.4.1 11 Feb 2025)
- **Python 3.10.11** (Build script)
- **Windows SDK** (for Windows headers - auto-detected by clang)

**Environment Setup:**

**Windows PowerShell:**
```powershell
# Set OpenSSL root directory
$env:OPENSSL_ROOT_DIR = "C:/path/to/openssl"

# Optional: Set compiler flags (handled by CMake)
$env:CFLAGS = "-I C:/msys64/ucrt64/include"
$env:CXXFLAGS = "-I C:/msys64/ucrt64/include" 
$env:LDFLAGS = "-L C:/msys64/ucrt64/lib"
```

**Windows CMD:**
```cmd
# Set OpenSSL root directory
set OPENSSL_ROOT_DIR=C:/path/to/openssl

# Optional: Set compiler flags (handled by CMake)
set CFLAGS=-I C:/msys64/ucrt64/include
set CXXFLAGS=-I C:/msys64/ucrt64/include
set LDFLAGS=-L C:/msys64/ucrt64/lib
```

**Linux/macOS Bash:**
```bash
# Set OpenSSL root directory
export OPENSSL_ROOT_DIR=/path/to/openssl

# Optional: Set compiler flags (handled by CMake)
export CFLAGS="-I /usr/local/include"
export CXXFLAGS="-I /usr/local/include" 
export LDFLAGS="-L /usr/local/lib"
```

**Note:** Using clang (not MSVC) - no Visual Studio paths needed. clang automatically finds standard library and Windows SDK headers.

------------------------------------------------------------------------------------------------------------------------------------------
# BUILD
## Debugging build script available in launch.json file
- Keybindings for build using vscode tasks
    Path:
      Win: C:\Users\<YourUsername>\AppData\Roaming\Code\User\keybindings.json
      MacOS: ~/Library/Application Support/Code/User/keybindings.json
      Linux: ~/.config/Code/User/keybindings.json

    Paste the below json in keybindings.json edit keybindings if required:
      [
        {
          "key": "ctrl+alt+r",
          "command": "workbench.action.tasks.runTask",
          "args": "Build Encrypter"
        },
        {
          "key": "ctrl+alt+t",
          "command": "workbench.action.tasks.runTask",
          "args": "Encrypter CMake Clean Build"
        },
        {
          "key": "ctrl+alt+e",
          "command": "workbench.action.tasks.runTask",
          "args": "Encrypter Fresh Rebuild"
        }
      ]



------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Observation in Windows for file watcher using neovim
    * nvim creates a temporary file from original file and write data on it
    * when we save using :write or :w it first deletes the original file
    * then renames the temporary file to original file.
    * this creates two events i.e delete and create. 
        Try to avoid using neovim to edit the file we're wathing in windows.
