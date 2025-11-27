#!/bin/bash
# ImAnim Bootstrap Script
# Builds Sharpmake and generates Visual Studio projects

echo "========================================"
echo "ImAnim Bootstrap Script"
echo "========================================"

# Check if Sharpmake submodule exists
if [ ! -d "extern/Sharpmake" ]; then
    echo ""
    echo "Sharpmake submodule not found!"
    echo "Please run: git submodule update --init --recursive"
    exit 1
fi

# Build Sharpmake
echo ""
echo "Step 1: Building Sharpmake..."
echo "========================================"
cd extern/Sharpmake

# Check for dotnet
if ! command -v dotnet &> /dev/null; then
    echo "Error: .NET SDK not found. Please install .NET 6.0 or later from https://dotnet.microsoft.com/download"
    cd ../..
    exit 1
fi

# Build Sharpmake.Application
echo "Building Sharpmake.Application..."
dotnet build Sharpmake.Application/Sharpmake.Application.csproj -c Release
if [ $? -ne 0 ]; then
    echo "Error: Failed to build Sharpmake"
    cd ../..
    exit 1
fi

cd ../..

# Copy Sharpmake artifacts to tools/sharpmake
echo ""
echo "Step 2: Copying Sharpmake artifacts to tools/sharpmake..."
echo "========================================"

mkdir -p "tools/sharpmake"

# Copy all DLLs and EXE from Sharpmake build output
SHARPMAKE_BIN="extern/Sharpmake/Sharpmake.Application/bin/x64/Release/net6.0"

if [ ! -d "$SHARPMAKE_BIN" ]; then
    echo "Error: Sharpmake build output not found at $SHARPMAKE_BIN"
    exit 1
fi

echo "Copying from $SHARPMAKE_BIN to tools/sharpmake..."
cp "$SHARPMAKE_BIN"/*.dll "tools/sharpmake/" 2>/dev/null
cp "$SHARPMAKE_BIN"/*.exe "tools/sharpmake/" 2>/dev/null
cp "$SHARPMAKE_BIN"/*.json "tools/sharpmake/" 2>/dev/null

echo "Sharpmake artifacts copied successfully!"

# Generate Visual Studio projects
echo ""
echo "Step 3: Generating Visual Studio projects..."
echo "========================================"
./generate_projects.sh
if [ $? -ne 0 ]; then
    echo "Error: Failed to generate projects"
    exit 1
fi

echo ""
echo "========================================"
echo "Bootstrap complete!"
echo "========================================"
echo ""
echo "You can now open the generated solution file:"
echo "  ImAnim_vs2022_win64.sln"
echo ""
