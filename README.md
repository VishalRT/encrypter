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
