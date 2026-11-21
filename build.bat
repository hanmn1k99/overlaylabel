@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"
if not exist build mkdir build

rem If OverlayLabel.exe is currently running, rename it aside so the linker can write a fresh .exe
if exist build\OverlayLabel.old.exe del /f /q build\OverlayLabel.old.exe >nul 2>&1
if exist build\OverlayLabel.exe (
    del /f /q build\OverlayLabel.exe >nul 2>&1
    if exist build\OverlayLabel.exe (
        move /y build\OverlayLabel.exe build\OverlayLabel.old.exe >nul 2>&1
    )
)

rem 1. Check if MSVC cl.exe is already available in PATH
where cl >nul 2>&1
if %errorlevel%==0 goto :build_msvc

rem 2. Try locating Visual Studio / Build Tools via vswhere.exe
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
            echo [INFO] Found MSVC at !VS_PATH!
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
            goto :build_msvc
        )
    )
)

rem 3. Try MinGW-w64 g++
where g++ >nul 2>&1
if %errorlevel%==0 (
    set "GPP=g++"
    set "WINDRES=windres"
    goto :build_mingw
)
for %%p in ("C:\msys64\ucrt64\bin" "C:\msys64\mingw64\bin" "C:\mingw64\bin" "%LOCALAPPDATA%\Microsoft\WinGet\Links") do (
    if exist "%%~p\g++.exe" (
        set "PATH=%%~p;%PATH%"
        set "GPP=%%~p\g++.exe"
        set "WINDRES=%%~p\windres.exe"
        goto :build_mingw
    )
)
for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*") do (
    if exist "%%~d\mingw64\bin\g++.exe" (
        set "PATH=%%~d\mingw64\bin;%PATH%"
        set "GPP=%%~d\mingw64\bin\g++.exe"
        set "WINDRES=%%~d\mingw64\bin\windres.exe"
        goto :build_mingw
    )
)

echo [ERROR] Khong tim thay trinh bien dich C++ (MSVC hoac MinGW-w64 g++).
echo Hay cai 1 trong 2 cong cu sau roi chay lai build.bat:
echo   - MinGW-w64 (nhanh, nhe): winget install BrechtSanders.WinLibs.POSIX.UCRT
echo   - Visual Studio Build Tools (C++): winget install Microsoft.VisualStudio.2022.BuildTools
exit /b 1

:build_msvc
echo [BUILD] Compiling with MSVC...
rc /nologo /fo build\app.res resources\app.rc
if errorlevel 1 exit /b 1

cl /nologo /std:c++17 /O2 /MT /W4 /utf-8 /EHsc /DUNICODE /D_UNICODE ^
    src\main.cpp src\config.cpp src\autostart.cpp src\overlay.cpp src\settings_window.cpp src\label_printer.cpp src\print_dialog.cpp ^
    build\app.res ^
    /Fo:build\ /Fe:build\OverlayLabel.exe ^
    /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib comctl32.lib comdlg32.lib ole32.lib
if errorlevel 1 exit /b 1

echo [SUCCESS] Built: %cd%\build\OverlayLabel.exe
exit /b 0

:build_mingw
echo [BUILD] Compiling with MinGW-w64...
"%WINDRES%" -I resources resources\app.rc -O coff -o build\app.res
if errorlevel 1 exit /b 1

"%GPP%" -std=c++17 -O2 -s -Wall -Wextra -DUNICODE -D_UNICODE -municode -mwindows ^
    src\main.cpp src\config.cpp src\autostart.cpp src\overlay.cpp src\settings_window.cpp src\label_printer.cpp src\print_dialog.cpp build\app.res ^
    -o build\OverlayLabel.exe ^
    -static -static-libgcc -static-libstdc++ ^
    -luser32 -lgdi32 -lgdiplus -lshell32 -ladvapi32 -lcomctl32 -lcomdlg32 -lole32
if errorlevel 1 exit /b 1

echo [SUCCESS] Built: %cd%\build\OverlayLabel.exe
exit /b 0
