#!/bin/bash

# Check if version argument is set
if [ -z "$1" ]; then
  echo "Error: You must include a version argument in command."
  echo "Eg: ./build.sh v0.6.8"
  exit 1
fi

VERSION=$1
VERSION_NO_V="${VERSION//v/}"
APPIMAGETOOL="./appimagetool-x86_64.AppImage"
APPDIR="PiozaLauncher.AppDir"

# Download appimagetool if missing
if [ ! -f "$APPIMAGETOOL" ]; then
  echo "appimagetool not found locally. Downloading..."
  curl -L https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage -o "$APPIMAGETOOL"
  chmod +x "$APPIMAGETOOL"
fi

echo "Stripping debug symbols from binaries..."
find "$APPDIR" -type f -name "*.so" -exec sh -c 'strip --strip-unneeded "$1" 2>/dev/null || true' sh {} \;
find "$APPDIR" -type f -perm -111 -exec sh -c 'file "$1" | grep -q "ELF" && strip --strip-unneeded "$1" 2>/dev/null || true' sh {} \;

# Install UPX if missing
if ! command -v upx &> /dev/null; then
  echo "UPX not found. Attempting to install..."

  if command -v apt-get &> /dev/null; then
    sudo apt-get update && sudo apt-get install -y upx-ucl
  elif command -v dnf &> /dev/null; then
    sudo dnf install -y upx
  elif command -v pacman &> /dev/null; then
    sudo pacman -Sy --noconfirm upx
  elif command -v zypper &> /dev/null; then
    sudo zypper install -y upx
  else
    echo "Unsupported package manager. Please install 'upx' manually."
    exit 1
  fi
fi

echo "Compressing ELF binaries with UPX..."
find "$APPDIR" -type f \( -name "*.so" -o -perm -111 \) -exec sh -c 'file "$1" | grep -q "ELF" && upx "$1" 2>/dev/null || true' sh {} \;

echo "Generating AppImage for version $VERSION..."
./"$APPIMAGETOOL" "$APPDIR"

mv Pioza_Launcher-x86_64.AppImage "PiozaGL-v$VERSION_NO_V-x86_64.AppImage"
echo "AppImage was created as: PiozaGL-v$VERSION_NO_V-x86_64.AppImage"
