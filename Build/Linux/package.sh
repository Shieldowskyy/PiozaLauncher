#!/bin/bash

# Script is in Build/Linux/package.sh, so project is 2 levels up
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
PROJECT_FILE="$PROJECT_DIR/PiozaGameLauncher.uproject"
CONFIG_FILE="$PROJECT_DIR/Config/DefaultEngine.ini"
OUTPUT_DIR="$PROJECT_DIR/Saved/Builds/Linux"

# Find RunUAT
POSSIBLE_UAT_PATHS=(
    "$HOME/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh"
    "/opt/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh"
    "$HOME/Epic/UE_5.5/Engine/Build/BatchFiles/RunUAT.sh"
    "/usr/local/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh"
)

UAT=""
for path in "${POSSIBLE_UAT_PATHS[@]}"; do
    if [ -f "$path" ]; then
        UAT="$path"
        break
    fi
done

if [ -z "$UAT" ]; then
    echo "RunUAT.sh not found!"
    exit 1
fi

echo "=== Backing up config and switching to ES31 ==="
cp "$CONFIG_FILE" "$CONFIG_FILE.bak"

# Switch to ES31
sed -i 's/+TargetedRHIs=SF_VULKAN_SM5/-TargetedRHIs=SF_VULKAN_SM5/' "$CONFIG_FILE"
grep -q "SF_VULKAN_ES31" "$CONFIG_FILE" || sed -i '/LinuxTargetSettings/a +TargetedRHIs=SF_VULKAN_ES31' "$CONFIG_FILE"

echo "=== Packaging for Linux (ES31) - Shipping only ==="
"$UAT" BuildCookRun \
    -project="$PROJECT_FILE" \
    -platform=Linux \
    -clientconfig=Shipping \
    -cook \
    -stage \
    -pak \
    -archive \
    -archivedirectory="$OUTPUT_DIR" \
    -nocompileeditor \
    -skipbuildeditor \
    -build \
    -clean

# Restore config
echo "=== Restoring SM5 ==="
mv "$CONFIG_FILE.bak" "$CONFIG_FILE"

echo "=== DONE! Build output: $OUTPUT_DIR ==="
