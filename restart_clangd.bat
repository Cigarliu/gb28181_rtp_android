@echo off
echo Stopping clangd processes...
taskkill /F /IM clangd.exe /T 2>nul
echo Clearing clangd cache...
if exist "%USERPROFILE%\.cache\clangd" (
    rmdir /S /Q "%USERPROFILE%\.cache\clangd"
)
if exist ".cache\clangd" (
    rmdir /S /Q ".cache\clangd"
)
echo Done. Please restart VS Code to reload clangd.
pause