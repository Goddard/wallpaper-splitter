#!/bin/bash

# Wallpaper Splitter Build Script
# This script automates the build process for the wallpaper splitter application

set -e  # Exit on any error

echo "=== Wallpaper Splitter Build Script ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake (using Ninja generator)
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja

# Build
echo "Building..."
ninja

echo "=== Build completed successfully! ==="
echo ""
echo "You can now run the applications:"
echo "  GUI: ./wallpaper-splitter-kde"
echo "  CLI: ./wallpaper-splitter-cli"
echo ""
echo "To install system-wide, run:"
echo "  sudo ninja install" 