@echo off
setlocal

REM 检查NDK环境
echo 正在检查NDK环境...

REM 检查ANDROID_HOME环境变量
if "%ANDROID_HOME%"=="" (
    echo ANDROID_HOME环境变量未设置
) else (
    echo ANDROID_HOME = %ANDROID_HOME%
)

REM 检查ANDROID_NDK_HOME环境变量
if "%ANDROID_NDK_HOME%"=="" (
    echo ANDROID_NDK_HOME环境变量未设置
) else (
    echo ANDROID_NDK_HOME = %ANDROID_NDK_HOME%
)

REM 检查SDK目录下的NDK
set SDK_NDK_PATH=C:\Users\%USERNAME%\AppData\Local\Android\Sdk\ndk

if exist "%SDK_NDK_PATH%" (
    echo 在SDK目录下找到NDK:
    dir /b "%SDK_NDK_PATH%"
) else (
    echo 在SDK目录下未找到NDK
)

REM 检查ndk-build命令
where ndk-build >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo ndk-build命令可用
    echo ndk-build路径:
    where ndk-build
) else (
    echo ndk-build命令不可用
)

REM 检查CMake命令
where cmake >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo CMake命令可用
    echo CMake版本:
    cmake --version
) else (
    echo CMake命令不可用
)

REM 检查Ninja命令
where ninja >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Ninja命令可用
    echo Ninja版本:
    ninja --version
) else (
    echo Ninja命令不可用
)

echo.
echo 环境检查完成
echo.

endlocal
pause