@echo off
REM ImAnim Bootstrap Script
REM Builds Sharpmake and generates Visual Studio projects

echo ========================================
echo ImAnim Bootstrap Script
echo ========================================

REM Check if Sharpmake submodule exists
if not exist "extern\Sharpmake" (
    echo.
    echo Sharpmake submodule not found!
    echo Please run: git submodule update --init --recursive
    exit /b 1
)

REM Build Sharpmake
echo.
echo Step 1: Building Sharpmake...
echo ========================================
cd extern\Sharpmake

REM Check for dotnet
where dotnet >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: .NET SDK not found. Please install .NET 6.0 or later from https://dotnet.microsoft.com/download
    cd ..\..
    exit /b 1
)

REM Build Sharpmake.Application
echo Building Sharpmake.Application...
dotnet build Sharpmake.Application\Sharpmake.Application.csproj -c Release
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to build Sharpmake
    cd ..\..
    exit /b 1
)

cd ..\..

REM Copy Sharpmake artifacts to tools/sharpmake
echo.
echo Step 2: Copying Sharpmake artifacts to tools\sharpmake...
echo ========================================

if not exist "tools\sharpmake" mkdir "tools\sharpmake"

REM Copy all DLLs and EXE from Sharpmake build output
set SHARPMAKE_BIN=extern\Sharpmake\Sharpmake.Application\bin\x64\Release\net6.0

if not exist "%SHARPMAKE_BIN%" (
    echo Error: Sharpmake build output not found at %SHARPMAKE_BIN%
    exit /b 1
)

echo Copying from %SHARPMAKE_BIN% to tools\sharpmake...
xcopy /Y /Q "%SHARPMAKE_BIN%\*.dll" "tools\sharpmake\" >nul
xcopy /Y /Q "%SHARPMAKE_BIN%\*.exe" "tools\sharpmake\" >nul
xcopy /Y /Q "%SHARPMAKE_BIN%\*.json" "tools\sharpmake\" >nul

if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to copy Sharpmake artifacts
    exit /b 1
)

echo Sharpmake artifacts copied successfully!

REM Generate Visual Studio projects
echo.
echo Step 3: Generating Visual Studio projects...
echo ========================================
call generate_projects.bat
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to generate projects
    exit /b 1
)

echo.
echo ========================================
echo Bootstrap complete!
echo ========================================
echo.
echo You can now open the generated solution file:
echo   ImAnim_vs2022_win64.sln
echo.
echo Or build from command line:
echo   msbuild ImAnim_vs2022_win64.sln /p:Configuration=Debug
echo.
