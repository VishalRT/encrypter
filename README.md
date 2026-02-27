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
2. Create `settings.json` in workspace root (already created):
```json
{
    "C_Cpp.default.configurationProvider": "clangd",
    "clangd.arguments": [
        "--background-index",
        "--clang-tidy",
        "--header-insertion=never"
    ],
    "C_Cpp.codeAnalysis.enabled": true,
    "C_Cpp.codeAnalysis.clangTidy.fallbackConfig": {
        "Checks": "-*,readability-identifier-naming",
        "CheckOptions": [
            {"key": "readability-identifier-naming.FunctionCase", "value": "snake_case"},
            {"key": "readability-identifier-naming.VariableCase", "value": "snake_case"},
            {"key": "readability-identifier-naming.ConstantCase", "value": "UPPER_CASE"},
            {"key": "readability-identifier-naming.ParameterCase", "value": "snake_case"},
            {"key": "readability-identifier-naming.ClassCase", "value": "PascalCase"},
            {"key": "readability-identifier-naming.StructCase", "value": "PascalCase"},
            {"key": "readability-identifier-naming.EnumCase", "value": "PascalCase"},
            {"key": "readability-identifier-naming.MemberCase", "value": "snake_case"},
            {"key": "readability-identifier-naming.PrivateMemberSuffix", "value": "_"}
        ]
    }
}
```
3. Restart VS Code
4. Real-time naming violations will appear as squiggles and in Problems panel

## Build-time Enforcement
- `.clang-tidy` file configured for naming rules
- Run `clang-tidy` manually or via build for full analysis 



----------------------------------------------------------------------------------------------------------------------------------------
# PREREQUISTE
- Add windows sdk include and lib in  CFLAGS , CXXFLAGS, LDFLAGS
    PS C:\msys64\ucrt64\lib> echo $env:CXXFLAGS
    -I C:\msys64\ucrt64\include
    PS C:\msys64\ucrt64\lib> echo $env:CFLAGS
    -I C:\msys64\ucrt64\include
    PS C:\msys64\ucrt64\lib> echo $env:LDFLAGS
    -I C:\msys64\ucrt64\lib
- OpenSSL 3.4.1 11 Feb 2025 (Library: OpenSSL 3.4.1 11 Feb 2025)
- I am using openssl in mingw. 
  SO the above CXX and LD Flags will give me the ssl libs.
  Included OPENSSL_ROOT_DIR in CMake for helping it find OpenSSL.
  Set the same(OPENSSL_ROOT_DIR ) environment variable which is root directory of installation
  Note: bin directory might not be root directory of installation
- Python version : 3.10.11, Used for building the project.

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
