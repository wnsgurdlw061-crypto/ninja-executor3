@echo off
echo ============================================
echo    NINJA ENGINE - Simple Build Script
echo ============================================
echo.

echo [1/3] Checking for compilers...
where cl.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [OK] Visual Studio found
    set USE_VS=1
    goto :build_vs
)

where gcc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [OK] MinGW found
    set USE_MINGW=1
    goto :build_mingw
)

echo [ERROR] No compiler found!
echo.
echo Please install:
echo - Visual Studio 2022 (with C++ workload)
echo - OR MinGW-w64
echo.
echo Download links:
echo VS: https://visualstudio.microsoft.com/downloads/
echo MinGW: https://github.com/brechtsanders/winlibs_mingw/releases
echo.
pause
exit /b 1

:build_vs
echo.
echo [2/3] Building with Visual Studio...
call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath > vs_path.txt
set /p VS_PATH=<vs_path.txt
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist cpp_engine\build\Release mkdir cpp_engine\build\Release

cl.exe /c /nologo /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D _USRDLL /D NINJA_ENGINE_EXPORTS /EHsc /MD /Fo"cpp_engine\build\Release\\" /I"cpp_engine" cpp_engine\engine_complete.cpp cpp_engine\veh_security.cpp >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed
    pause
    exit /b 1
)

link.exe /nologo /DLL /OUT:"cpp_engine\build\Release\ninja_engine.dll" cpp_engine\build\Release\engine_complete.obj cpp_engine\build\Release\veh_security.obj kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32 >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Linking failed
    pause
    exit /b 1
)

echo [OK] DLL built with Visual Studio
goto :copy_files

:build_mingw
echo.
echo [2/3] Building with MinGW...
if not exist cpp_engine\build\Release mkdir cpp_engine\build\Release

g++ -shared -O2 -static-libgcc -static-libstdc++ -DWIN32 -DNDEBUG -D_WINDOWS -D_USRDLL -DNINJA_ENGINE_EXPORTS -I"cpp_engine" cpp_engine\engine_complete.cpp cpp_engine\veh_security.cpp -o cpp_engine\build\Release\ninja_engine.dll -lkernel32 -luser32 -lgdi32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lws2_32 >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed
    pause
    exit /b 1
)

echo [OK] DLL built with MinGW
goto :copy_files

:copy_files
echo.
echo [3/3] Copying to Electron app...
if exist dist\NinjaExecutor-win32-x64 (
    mkdir dist\NinjaExecutor-win32-x64\engine 2>nul
    copy /Y cpp_engine\build\Release\ninja_engine.dll dist\NinjaExecutor-win32-x64\engine\ >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo [OK] Engine copied to app
    ) else (
        echo [WARNING] Could not copy engine
    )
)

echo.
echo ============================================
echo              BUILD COMPLETE
echo ============================================
echo.
echo Output: cpp_engine\build\Release\ninja_engine.dll
echo.
if exist dist\NinjaExecutor-win32-x64\engine\ninja_engine.dll (
    echo [SUCCESS] Ready for real game usage!
    echo Run: dist\NinjaExecutor-win32-x64\NinjaExecutor.exe
) else (
    echo [INFO] Manual copy required:
    echo   copy cpp_engine\build\Release\ninja_engine.dll dist\NinjaExecutor-win32-x64\engine\
)
echo.
pause
del vs_path.txt >nul 2>&1
