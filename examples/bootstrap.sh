#!/bin/bash
# ImAnim Bootstrap Script
# Builds Sharpmake and generates projects

set -e

echo "========================================"
echo "ImAnim Bootstrap Script"
echo "========================================"

# Save the examples directory
EXAMPLES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$EXAMPLES_DIR"

# Check if Sharpmake submodule exists and is initialized
if [ ! -f "extern/Sharpmake/Sharpmake.sln" ]; then
    echo ""
    echo "Sharpmake submodule not initialized!"
    echo "Initializing submodules..."
    cd ..
    git submodule update --init --recursive
    if [ $? -ne 0 ]; then
        echo "Error: Failed to initialize submodules"
        echo "Please run manually: git submodule update --init --recursive"
        cd "$EXAMPLES_DIR"
        exit 1
    fi
    cd "$EXAMPLES_DIR"
fi

# Build Sharpmake
echo ""
echo "Step 1: Building Sharpmake..."
echo "========================================"
cd extern/Sharpmake

# Check for dotnet
if ! command -v dotnet &> /dev/null; then
    echo "Error: .NET SDK not found. Please install .NET 6.0 or later from https://dotnet.microsoft.com/download"
    cd "$EXAMPLES_DIR"
    exit 1
fi

# Build Sharpmake.Application
echo "Building Sharpmake.Application..."
dotnet build Sharpmake.Application/Sharpmake.Application.csproj -c Release
if [ $? -ne 0 ]; then
    echo "Error: Failed to build Sharpmake"
    cd "$EXAMPLES_DIR"
    exit 1
fi

cd "$EXAMPLES_DIR"

# Copy Sharpmake artifacts to tools/sharpmake
echo ""
echo "Step 2: Copying Sharpmake artifacts to tools/sharpmake..."
echo "========================================"

mkdir -p "tools/sharpmake"

# Sharpmake build output path (platform-independent)
SHARPMAKE_BIN="extern/Sharpmake/Sharpmake.Application/bin/Release/net6.0"

# Try x64 path first (Windows build), then generic path (Linux/macOS)
if [ ! -d "$SHARPMAKE_BIN" ]; then
    SHARPMAKE_BIN="extern/Sharpmake/Sharpmake.Application/bin/x64/Release/net6.0"
fi

if [ ! -d "$SHARPMAKE_BIN" ]; then
    echo "Error: Sharpmake build output not found"
    echo "Checked paths:"
    echo "  - extern/Sharpmake/Sharpmake.Application/bin/Release/net6.0"
    echo "  - extern/Sharpmake/Sharpmake.Application/bin/x64/Release/net6.0"
    exit 1
fi

echo "Copying from $SHARPMAKE_BIN to tools/sharpmake..."
cp -f "$SHARPMAKE_BIN"/*.dll "tools/sharpmake/" 2>/dev/null || true
cp -f "$SHARPMAKE_BIN"/*.exe "tools/sharpmake/" 2>/dev/null || true
cp -f "$SHARPMAKE_BIN"/*.json "tools/sharpmake/" 2>/dev/null || true
cp -f "$SHARPMAKE_BIN"/Sharpmake.Application "tools/sharpmake/" 2>/dev/null || true

echo "Sharpmake artifacts copied successfully!"

# Generate projects
echo ""
echo "Step 3: Generating projects..."
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
echo "You can now open the generated solution/project files."
echo ""
