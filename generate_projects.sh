#!/bin/bash
# ImAnim Project Generation Script
# Invokes Sharpmake to generate Visual Studio projects

echo "Generating ImAnim Visual Studio projects..."

if [ ! -f "tools/sharpmake/Sharpmake.Application.exe" ]; then
    echo "Error: Sharpmake not found in tools/sharpmake/"
    echo "Please run bootstrap.sh first"
    exit 1
fi

./tools/sharpmake/Sharpmake.Application.exe /sources\('sharpmake/main.cs'\)

if [ $? -ne 0 ]; then
    echo "Error: Failed to generate projects"
    exit 1
fi

echo "Projects generated successfully!"
