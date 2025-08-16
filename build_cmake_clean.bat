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

REM Verify NDK path exists
if not exist "%NDK_PATH%" (
    echo Error: NDK path does not exist: %NDK_PATH%
    pause
    exit /b 1
)

REM Verify Android toolchain file exists
if not exist "%NDK_PATH%\build\cmake\android.toolchain.cmake" (
    echo Error: Android toolchain file not found
    pause
    exit /b 1
)

REM Completely remove build directory
if exist build (
    echo Removing existing build directory...
    rmdir /s /q build
    timeout /t 2 /nobreak >nul
)

REM Create fresh build directory
mkdir build
cd build

REM Set environment variables to force NDK usage
set CC=%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android21-clang.exe
set CXX=%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android21-clang++.exe
set AR=%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-ar.exe
set STRIP=%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe

echo Using compilers:
echo CC: %CC%
echo CXX: %CXX%

REM Configure project with CMake for Android cross-compilation
echo Configuring CMake for Android...
cmake .. ^
    -G "Unix Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE="%NDK_PATH%\build\cmake\android.toolchain.cmake" ^
    -DCMAKE_C_COMPILER="%CC%" ^
    -DCMAKE_CXX_COMPILER="%CXX%" ^
    -DCMAKE_AR="%AR%" ^
    -DCMAKE_STRIP="%STRIP%" ^
    -DANDROID_ABI=%ABI% ^
    -DANDROID_PLATFORM=%ANDROID_PLATFORM% ^
    -DANDROID_NDK="%NDK_PATH%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DANDROID_STL=c++_static ^
    -DANDROID_CPP_FEATURES="rtti exceptions" ^
    -DCMAKE_MAKE_PROGRAM="%NDK_PATH%\prebuilt\windows-x86_64\bin\make.exe"

REM Check configuration result
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

REM Build project
echo Building project with %NUMBER_OF_PROCESSORS% cores...
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%

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
