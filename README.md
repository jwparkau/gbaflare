# GBAFlare
An emulator for the Game Boy Advance.

## Building
### Dependencies
- CMake
- Qt

The `configure.sh` and `configure.bat` scripts will use the default generator for your platform.

### On Linux (using Unix Makefiles)
1. `./configure.sh "Unix Makefiles" && cd build`
2. `make`

### On Windows (using Visual Studio 17 2022)
1. Determine the directory where Qt is located. The following steps will use `"C:\Qt\5.15.2\msvc2019_64"`.
2. `cmake -Bbuild -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" -G "Visual Studio 17 2022"`
3. Open the `.sln` file which has been generated inside the `build` directory.
4. Change the solution configuration from Debug to Release.
5. Build -> Build Solution.

The executable will be located in the `build\Release` directory.
#### Notes
- To run the executable the necessary `.dll` files must be found. The easiest way to do this is to add `"C:\Qt\5.15.2\msvc2019_64\bin"` to your PATH.

### Using Qt Creator
Open `CMakeLists.txt` and it should work out of the box.
