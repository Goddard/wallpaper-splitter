#!/bin/bash

# Wallpaper Splitter Icon Generator
# This script generates all required icon sizes

set -e  # Exit on any error

echo "=== Wallpaper Splitter Icon Generator ==="

# Check if ImageMagick is available
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick 'convert' command not found."
    echo "Please install ImageMagick:"
    echo "  sudo pacman -S imagemagick"
    exit 1
fi

# Check if original icon exists
if [ ! -f "original-icon-file-transparent.png" ]; then
    echo "Error: original-icon-file-transparent.png not found in current directory."
    echo "Please place your original icon file in the project root directory."
    exit 1
fi

echo "Generating icon sizes..."

# Generate all required icon sizes
echo "Creating 16x16 icon..."
convert original-icon-file-transparent.png -resize 16x16 original-icon-file-transparent-16.png

echo "Creating 32x32 icon..."
convert original-icon-file-transparent.png -resize 32x32 original-icon-file-transparent-32.png

echo "Creating 48x48 icon..."
convert original-icon-file-transparent.png -resize 48x48 original-icon-file-transparent-48.png

echo "Creating 64x64 icon..."
convert original-icon-file-transparent.png -resize 64x64 original-icon-file-transparent-64.png

echo "Creating 96x96 icon..."
convert original-icon-file-transparent.png -resize 96x96 original-icon-file-transparent-96.png

echo "Creating 128x128 icon..."
convert original-icon-file-transparent.png -resize 128x128 original-icon-file-transparent-128.png

echo "Creating 256x256 icon..."
convert original-icon-file-transparent.png -resize 256x256 original-icon-file-transparent-256.png

echo "Creating 512x512 icon..."
convert original-icon-file-transparent.png -resize 512x512 original-icon-file-transparent-512.png

echo ""
echo "=== Icon generation complete! ==="
echo ""
echo "Generated files:"
ls -la original-icon-file-transparent-*.png
echo ""
echo "You can now rebuild the Flatpak with:"
echo "  flatpak-builder --user --install --force-clean build-dir org.wallpapersplitter.app.yml"
echo ""
echo "Note: The original-icon-file-transparent-512.png file is already in your .gitignore"
echo "so it won't be tracked by git. The other generated files are also ignored." 