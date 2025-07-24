# GB28181 JNI 环境测试脚本

Write-Host "===== GB28181 JNI 环境测试 =====" -ForegroundColor Cyan

# 检查环境变量
Write-Host "\n[检查环境变量]" -ForegroundColor Yellow

# 检查ANDROID_HOME
if ($env:ANDROID_HOME) {
    Write-Host "ANDROID_HOME = $env:ANDROID_HOME" -ForegroundColor Green
} else {
    Write-Host "ANDROID_HOME 环境变量未设置" -ForegroundColor Red
}

# 检查ANDROID_NDK_HOME
if ($env:ANDROID_NDK_HOME) {
    Write-Host "ANDROID_NDK_HOME = $env:ANDROID_NDK_HOME" -ForegroundColor Green
} else {
    Write-Host "ANDROID_NDK_HOME 环境变量未设置" -ForegroundColor Red
}

# 检查SDK目录下的NDK
Write-Host "\n[检查SDK目录下的NDK]" -ForegroundColor Yellow
$sdkNdkPath = "C:\Users\$env:USERNAME\AppData\Local\Android\Sdk\ndk"

if (Test-Path $sdkNdkPath) {
    $ndkVersions = Get-ChildItem $sdkNdkPath -Directory | Select-Object -ExpandProperty Name
    Write-Host "在SDK目录下找到以下NDK版本:" -ForegroundColor Green
    foreach ($version in $ndkVersions) {
        Write-Host "  - $version" -ForegroundColor Green
    }
    
    # 获取最新版本
    $latestVersion = $ndkVersions | Sort-Object -Descending | Select-Object -First 1
    Write-Host "最新版本: $latestVersion" -ForegroundColor Green
    
    # 检查ndk-build.cmd是否存在
    $ndkBuildPath = "$sdkNdkPath\$latestVersion\ndk-build.cmd"
    if (Test-Path $ndkBuildPath) {
        Write-Host "ndk-build.cmd 存在于: $ndkBuildPath" -ForegroundColor Green
    } else {
        Write-Host "在最新版本中未找到 ndk-build.cmd" -ForegroundColor Red
    }
} else {
    Write-Host "在SDK目录下未找到NDK" -ForegroundColor Red
}

# 检查命令可用性
Write-Host "\n[检查命令可用性]" -ForegroundColor Yellow

# 检查ndk-build
$ndkBuild = Get-Command ndk-build -ErrorAction SilentlyContinue
if ($ndkBuild) {
    Write-Host "ndk-build 命令可用: $($ndkBuild.Source)" -ForegroundColor Green
} else {
    Write-Host "ndk-build 命令不可用" -ForegroundColor Red
}

# 检查CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    Write-Host "CMake 命令可用: $($cmake.Source)" -ForegroundColor Green
    $cmakeVersion = & cmake --version | Select-Object -First 1
    Write-Host "  版本: $cmakeVersion" -ForegroundColor Green
} else {
    Write-Host "CMake 命令不可用" -ForegroundColor Red
}

# 检查Ninja
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if ($ninja) {
    Write-Host "Ninja 命令可用: $($ninja.Source)" -ForegroundColor Green
    $ninjaVersion = & ninja --version
    Write-Host "  版本: $ninjaVersion" -ForegroundColor Green
} else {
    Write-Host "Ninja 命令不可用" -ForegroundColor Red
}

# 总结
Write-Host "\n[环境测试总结]" -ForegroundColor Yellow
Write-Host "1. 请确保使用正确的NDK路径编译项目" -ForegroundColor Cyan
Write-Host "2. 如果命令不可用，请考虑使用绝对路径调用编译工具" -ForegroundColor Cyan
Write-Host "3. 可以使用以下编译脚本:" -ForegroundColor Cyan
Write-Host "   - build.bat (NDK-Build)" -ForegroundColor Cyan
Write-Host "   - build_cmake.bat (CMake)" -ForegroundColor Cyan
Write-Host "   - build.ps1 (PowerShell脚本，支持更多选项)" -ForegroundColor Cyan

Write-Host "\n===== 环境测试完成 =====" -ForegroundColor Cyan

Write-Host "按任意键继续..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")