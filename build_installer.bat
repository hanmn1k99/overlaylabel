@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

echo [1/2] Building OverlayLabel.exe...
call build.bat
if errorlevel 1 exit /b 1

echo [2/2] Locating Inno Setup Compiler (ISCC.exe)...
set "ISCC="
where ISCC >nul 2>&1
if %errorlevel%==0 set "ISCC=ISCC"

if not defined ISCC if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"

if not defined ISCC (
    echo [ERROR] Khong tim thay Inno Setup 6 - ISCC.exe.
    echo Cai dat nhanh: winget install --id JRSoftware.InnoSetup -e
    exit /b 1
)

if not exist dist mkdir dist
"%ISCC%" installer\OverlayLabel.iss
if errorlevel 1 exit /b 1

echo [SUCCESS] Installer created in: %cd%\dist\OverlayLabel-Setup-1.1.0.exe
exit /b 0
