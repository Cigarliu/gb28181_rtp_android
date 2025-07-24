@echo off
setlocal

set "NDK_PATH=C:\Users\%USERNAME%\AppData\Local\Android\Sdk\ndk\26.1.10909125"

echo Checking for jni.h in NDK directory...

if exist "%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include\jni.h" (
    echo Found jni.h at: %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include\jni.h
) else (
    echo jni.h not found in expected location
)

echo.
echo Checking for android/log.h...

if exist "%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include\android\log.h" (
    echo Found android/log.h at: %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include\android\log.h
) else (
    echo android/log.h not found in expected location
)

echo.
echo Done.
pause