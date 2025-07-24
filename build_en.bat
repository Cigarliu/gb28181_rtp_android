@echo off
setlocal

REM Set NDK path
set "NDK_PATH=C:\Users\%USERNAME%\AppData\Local\Android\Sdk\ndk\26.1.10909125"

REM Set target platform
set "ABI=arm64-v8a"

REM Display build information
echo Using NDK path: %NDK_PATH%
echo Target platform: %ABI%

REM Change to jni directory
cd /d "%~dp0jni"

REM Compile using ndk-build
"%NDK_PATH%\ndk-build.cmd" NDK_PROJECT_PATH=.. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk APP_ABI=%ABI%

REM Check compilation result
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo Output files located at: %~dp0\libs\%ABI%\
) else (
    echo Compilation failed, error code: %ERRORLEVEL%
)

endlocal
pause