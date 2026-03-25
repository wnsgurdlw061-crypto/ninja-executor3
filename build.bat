@echo off
echo Building Ninja Engine DLL...

set BUILD_DIR=build

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

cd %BUILD_DIR%

echo Generating CMake build files...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building Release configuration...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo DLL location: %CD%\Release\ninja_engine.dll
echo.
pause
