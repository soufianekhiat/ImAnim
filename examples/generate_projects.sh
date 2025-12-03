#!/bin/bash
# ImAnim Project Generation Script
# Invokes Sharpmake to generate projects

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Generating ImAnim projects..."

# Check for Sharpmake - try both .exe (Mono/Wine) and native binary
SHARPMAKE_EXE=""
if [ -f "tools/sharpmake/Sharpmake.Application.exe" ]; then
    SHARPMAKE_EXE="tools/sharpmake/Sharpmake.Application.exe"
elif [ -f "tools/sharpmake/Sharpmake.Application" ]; then
    SHARPMAKE_EXE="tools/sharpmake/Sharpmake.Application"
else
    echo "Error: Sharpmake not found in tools/sharpmake/"
    echo "Please run bootstrap.sh first"
    exit 1
fi

# Run Sharpmake
if [[ "$SHARPMAKE_EXE" == *.exe ]]; then
    # .exe file - need dotnet or mono to run
    if command -v dotnet &> /dev/null; then
        dotnet "$SHARPMAKE_EXE" /sources\('sharpmake/main.cs'\)
    elif command -v mono &> /dev/null; then
        mono "$SHARPMAKE_EXE" /sources\('sharpmake/main.cs'\)
    else
        echo "Error: Neither dotnet nor mono found to run Sharpmake.Application.exe"
        exit 1
    fi
else
    # Native binary
    "./$SHARPMAKE_EXE" /sources\('sharpmake/main.cs'\)
fi

if [ $? -ne 0 ]; then
    echo "Error: Failed to generate projects"
    exit 1
fi

echo "Projects generated successfully!"
