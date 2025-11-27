@echo off
REM ImAnim Project Generation Script
REM Invokes Sharpmake to generate Visual Studio projects

echo Generating ImAnim Visual Studio projects...

if not exist "tools\sharpmake\Sharpmake.Application.exe" (
    echo Error: Sharpmake not found in tools\sharpmake\
    echo Please run bootstrap.bat first
    exit /b 1
)

.\tools\sharpmake\Sharpmake.Application.exe /sources('sharpmake/main.cs')

if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to generate projects
    exit /b 1
)

echo Projects generated successfully!
