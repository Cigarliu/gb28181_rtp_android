@echo off
setlocal

REM Set NDK path
set NDK_PATH=C:\Users\%USERNAME%\AppData\Local\Android\Sdk\ndk\26.1.10909125

REM Set target platform
set ABI=arm64-v8a
set ANDROID_PLATFORM=android-21

REM Display build information
echo Using NDK path: %NDK_PATH%
echo Target platform: %ABI%
echo Android platform: %ANDROID_PLATFORM%

REM Create and change to build directory
if not exist build mkdir build
cd build

REM Configure project with CMake
REM Using Ninja generator
cmake .. ^
    -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%NDK_PATH%\build\cmake\android.toolchain.cmake" ^
    -DANDROID_ABI=%ABI% ^
    -DANDROID_PLATFORM=%ANDROID_PLATFORM% ^
    -DCMAKE_BUILD_TYPE=Release

REM Build project
cmake --build . --config Release

REM Check compilation result
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo Output files located at: %~dp0\build\
) else (
    echo Compilation failed, error code: %ERRORLEVEL%
)

REM Return to original directory
cd ..

endlocal
pause