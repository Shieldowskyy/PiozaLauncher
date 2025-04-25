#!/bin/bash

# Check if version argument is set
if [ -z "$1" ]; then
  echo "Error: You must include a version argument in command."
  echo "Eg: ./build.sh v0.6.8"
  exit 1
fi

# Version param
VERSION=$1

# Remove additional "v" prefix if it exists
VERSION_NO_V="${VERSION//v/}"

# appimagetool path
APPIMAGETOOL="./appimagetool-x86_64.AppImage"

# Check if appimagetool is avaliable locally.
if [ ! -f "$APPIMAGETOOL" ]; then
  echo "appimagetool not found locally. Downloading..."
  # Download if not.
  curl -L https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage -o "$APPIMAGETOOL"
  chmod +x "$APPIMAGETOOL"
fi

# Set Appimage Folder
APPDIR="PiozaLauncher.AppDir"

echo "Stripping debug symbols from binaries..."
find . -type f -name "*.so" -exec sh -c 'strip --strip-unneeded "$1" 2>/dev/null || true' sh {} \;

# Generating AppImage
echo "Generating Appimage for version $VERSION..."
./"$APPIMAGETOOL" "$APPDIR"

# Change Appimage name to verisoned one.
mv Pioza_Launcher-x86_64.AppImage "PiozaLauncher-v$VERSION_NO_V-x86_64.AppImage"

# Finalization
echo "Appimage was created as: PiozaLauncher-v$VERSION_NO_V-x86_64.AppImage"
