# GB28181 JNI Build Script

# Parameter definitions
param (
    [string]$BuildSystem = "ndk-build", # Build system: ndk-build or cmake
    [string]$Abi = "arm64-v8a",        # Target ABI
    [string]$NdkVersion = "26.1.10909125" # NDK version
)

# NDK path
$NdkPath = "C:\Users\$env:USERNAME\AppData\Local\Android\Sdk\ndk\$NdkVersion"

# Check if NDK path exists
if (-not (Test-Path $NdkPath)) {
    Write-Host "Error: NDK path does not exist: $NdkPath" -ForegroundColor Red
    Write-Host "Please check if NDK is installed, or specify the correct NDK version" -ForegroundColor Red
    exit 1
}

# Display build information
Write-Host "===== GB28181 JNI Build Configuration =====" -ForegroundColor Cyan
Write-Host "Build system: $BuildSystem" -ForegroundColor Cyan
Write-Host "Target ABI: $Abi" -ForegroundColor Cyan
Write-Host "NDK path: $NdkPath" -ForegroundColor Cyan
Write-Host "==============================" -ForegroundColor Cyan

# Choose build method based on build system
if ($BuildSystem -eq "ndk-build") {
    # Compile using ndk-build
    Write-Host "Compiling using ndk-build..." -ForegroundColor Yellow
    
    # Change to jni directory
    Push-Location "$PSScriptRoot\jni"
    
    # Execute ndk-build command
    & "$NdkPath\ndk-build.cmd" NDK_PROJECT_PATH=.. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk APP_ABI=$Abi
    
    # Check compilation result
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
        Write-Host "Output files located at: $PSScriptRoot\libs\$Abi\" -ForegroundColor Green
    } else {
        Write-Host "Compilation failed, error code: $LASTEXITCODE" -ForegroundColor Red
    }
    
    # Return to original directory
    Pop-Location
} elseif ($BuildSystem -eq "cmake") {
    # Compile using CMake
    Write-Host "Compiling using CMake..." -ForegroundColor Yellow
    
    # Create and change to build directory
    if (-not (Test-Path "$PSScriptRoot\build")) {
        New-Item -Path "$PSScriptRoot\build" -ItemType Directory | Out-Null
    }
    Push-Location "$PSScriptRoot\build"
    
    # Configure project with CMake
    & cmake .. -G "Ninja" `
        -DCMAKE_TOOLCHAIN_FILE="$NdkPath\build\cmake\android.toolchain.cmake" `
        -DANDROID_ABI=$Abi `
        -DANDROID_PLATFORM=android-21 `
        -DCMAKE_BUILD_TYPE=Release
    
    # Build project
    & cmake --build .
    
    # Check compilation result
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Compilation successful!" -ForegroundColor Green
        Write-Host "Output files located at: $PSScriptRoot\build\" -ForegroundColor Green
    } else {
        Write-Host "Compilation failed, error code: $LASTEXITCODE" -ForegroundColor Red
    }
    
    # Return to original directory
    Pop-Location
} else {
    Write-Host "Error: Unsupported build system: $BuildSystem" -ForegroundColor Red
    Write-Host "Supported build systems: ndk-build, cmake" -ForegroundColor Red
    exit 1
}

Write-Host "Press any key to continue..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")