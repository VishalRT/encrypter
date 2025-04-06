- stop at cmake if there's any issue with it
- capture delta of file
- encrypt file to destination
- decrypt file
- When to encrypt and decrypt to make the file available to user ?
- Should we have some opening point to the file. i.e when file is opened we decrypt and open the txt file ? 
- Removed verbose log to build system, make it optional later



----------------------------------
PREREQUISTE
- Add windows sdk include and lib in  CFLAGS , CXXFLAGS, LDFLAGS
    PS C:\msys64\ucrt64\lib> echo $env:CXXFLAGS
-I C:\msys64\ucrt64\include
PS C:\msys64\ucrt64\lib> echo $env:CFLAGS
-I C:\msys64\ucrt64\include
PS C:\msys64\ucrt64\lib> echo $env:LDFLAGS
-I C:\msys64\ucrt64\lib
- 
- Python version : 3.10.11

------------------------------------
BUILD
- Keybindings for build using vscode tasks
    Path:
      Win: C:\Users\<YourUsername>\AppData\Roaming\Code\User\keybindings.json
      MacOS: ~/Library/Application Support/Code/User/keybindings.json
      Linux: ~/.config/Code/User/keybindings.json

    Paste the below json in keybindings.json:
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



------------------------------------
- Observation in Windows for file watcher using neovim
    * nvim creates a temporary file from original file and write data on it
    * when we save using :write or :w it first deletes the original file
    * then renames the temporary file to original file.
    * this creates two events i.e delete and create. 
        Try to avoid using neovim to edit the file we're wathing in windows.
