# TODO
- use mr stroustrup's guidelines from github to enforce best practices on identifiers - DONE
- implement cmake --fresh option - DONE
- Make CLI first then lets move to the watcher logic part.- DONE
- Service implementation, we should be able to observe file changes from service - DONE
- Modularize to files - DONE
- FileRename not working fix - InProgress
- Use `std::expected` (C++23) for cleaner error handling
- handle closeHandle on abrupt close of exe (ctrl + c?), Use `SetConsoleCtrlHandler` for graceful shutdown on Windows
- Implement RAII wrappers for OpenSSL and Windows handles
- encrypt file to specific destination
- OOP's application
- capture delta of file and update if required - for np++, vscode dual modified calls fix
- Don't use string in password. check if strings are immutable in cpp as well
- decrypt file and open using windows/default app, When to encrypt and decrypt to make the file available to user ?
- main cpp use decrypt(main.cpp:Line79) only when required
- Line 79 CmakeList.txt Include clang-tidy as part of cmake build system
- Should we have some opening point to the file. i.e when file is opened we decrypt and open the txt file ?
- Installation and initiazion of service in windows
- Implementaiton in other OS, Linux and MacOS
- CMAKE_GENERATOR env variable as flag to select build generator(ninja or make)
- stop at cmake if there's any issue with it during build
- Removed verbose log to build system, make it optional later
- GUI - Maybe last when most of the things are completed 

----------------------------------------------------------------------------------------------------------------------------------------

# Overview
- Simple application to encrypt file using AES-256-CBC with password-derived(PBKDF2) key 
- **NOTE: DO NOT USE THIS IS NOT SECURE YET**
- File watcher to encrypt based on file changes. Probably will create a OS service in future that reads writes from source to destination
- Why did I created this? Because I will use it.


# Dev Environment Setup

## PREREQUISTE
- **Clang 20.1.1** (C++23 compiler)
- **OpenSSL 3.4.1** (Library: OpenSSL 3.4.1 11 Feb 2025)
- **Python 3.10.11** (Build script)
- **VS Code Extension**: clangd with clang-tidy integration
- **Configuration**: `.clang-tidy` file for naming rules

**NOTE**: Version aren't mandatory used when this was being developed

## Windows 
- **Toolchain**: MSYS2 / MinGW-w64 (UCRT64)
- We are using msys2 to use clang and the supported win32 API's provided by minGW. So that we don't have to use clang by msvc.
- **Windows API Headers** : MinGW-w64 Headers v12.0.0 from Universal C Runtime (UCRT)

### Windows PowerShell:
```powershell
# Set OpenSSL root directory
$env:OPENSSL_ROOT_DIR = "C:/msys64/ucrt64"

# Add compiler/library paths to PATH if needed
$env:PATH = "C:/msys64/ucrt64/bin;$env:PATH"

# Optional: Set MSYS2 environment variables for UCRT64
$env:MSYSTEM = "UCRT64"
$env:MSYS2_PATH = "C:/msys64/ucrt64"
```

### Windows CMD:
```cmd
# Set OpenSSL root directory (Used by cmake to identify includes/libs)
set OPENSSL_ROOT_DIR=C:/msys64/ucrt64

# Add bin to path
set PATH=C:/msys64/ucrt64/bin;%PATH%

# OPTIONAL: Set OpenSSL root directory (Used by cmake to identify includes/libs)
set MSYSTEM=UCRT64
set MSYS2_PATH=C:/msys64/ucrt64
```

## Linux/macOS Bash:
```bash
TBU
```

------------------------------------------------------------------------------------------------------------------------------------------
## VS Code Setup
1. Install clangd extension from VS Code Marketplace
2. Create `settings.json` in workspace root (already created)
3. Real-time naming violations will appear as squiggles and in Problems panel

## Build-time Enforcement
- `.clang-tidy` file configured for naming rules
- **VS Code Intellisense**: VS Code with clangd extension
- **Conditional Build**: `python build.py --build-action rebuild --clang-tidy-check` (only when flag passed)
- **CMake Target**: `cmake --build build --target clang-tidy-check` (only when enabled)
- **Manual**: `clang-tidy src/main.cpp --checks='readability-identifier-naming'`

## Code Conventions ([C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-philosophy) NL Section)
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

# BUILD

## Build Command Details
```powershell
python build.py [--build-action {rebuild,clean,fresh}] [--build-type {debug,release,relwithdebinfo}] [--clang-tidy-check]
```
**NOTE:** Default values if not mentioned : clean release build without clang-tidy chec

**Build Actions:**
- **rebuild**: Normal incremental build
- **clean**: Clean build targets then rebuild  
- **fresh**: Complete rebuild with fresh configuration


**Build type**
- Just [cmake build types](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html), each of them aren't included
    


**Usage Examples:**
```bash
# Rebuild with naming checks
python build.py --build-action rebuild --clang-tidy-check

# Clean build without naming checks
python build.py --build-action clean

# Fresh build with naming checks
python build.py --build-action fresh --clang-tidy-check

# Fresh build without naming checks
python build.py --build-action fresh --build-type debug
``` 

## Debugging build script available in launch.json file
- Keybindings for build using vscode tasks
```
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
```


------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Observation in Windows for file watcher using neovim
    * nvim creates a temporary file from original file and write data on it
    * when we save using :write or :w it first deletes the original file
    * then renames the temporary file to original file.
    * this creates two events i.e delete and create. 
        Try to avoid using neovim to edit the file we're wathing in windows.
