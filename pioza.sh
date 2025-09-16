#!/usr/bin/env bash
set -e

APP_NAME="PiozaLauncher"
APP_DIR="$HOME/.pioza-launcher"
ICON_URL="https://github.com/Shieldowskyy/PiozaLauncher/raw/main/pioza.ico"
GITHUB_API="https://api.github.com/repos/Shieldowskyy/PiozaLauncher/releases/latest"

echo "Checking latest release of $APP_NAME..."

# --------------------------
# Create target directory
# --------------------------
mkdir -p "$APP_DIR"

# --------------------------
# Get latest AppImage
# --------------------------
LATEST_URL=$(curl -s $GITHUB_API | grep "browser_download_url.*AppImage" | cut -d '"' -f 4)
LATEST_FILE=$(basename "$LATEST_URL")

if [ -f "$APP_DIR/$LATEST_FILE" ]; then
    echo "You already have the latest version: $LATEST_FILE"
else
    echo "Downloading latest version: $LATEST_FILE..."
    curl -L "$LATEST_URL" -o "$APP_DIR/$LATEST_FILE"
    chmod +x "$APP_DIR/$LATEST_FILE"

    # Remove old versions
    for f in "$APP_DIR"/*.AppImage; do
        if [ "$f" != "$APP_DIR/$LATEST_FILE" ]; then
            rm "$f"
        fi
    done
fi

# --------------------------
# Download icon and convert to PNG
# --------------------------
ICON_FILE="$APP_DIR/pioza.png"
if [ ! -f "$ICON_FILE" ]; then
    echo "Downloading icon..."
    curl -L "$ICON_URL" -o "$APP_DIR/pioza.ico"

    # Convert .ico to .png (requires ImageMagick)
    if command -v convert &>/dev/null; then
        convert "$APP_DIR/pioza.ico" "$ICON_FILE"
    else
        echo "Warning: ImageMagick not found, cannot convert icon. Install 'imagemagick' or rename .ico to .png manually."
        ICON_FILE="$APP_DIR/pioza.ico"
    fi
fi

# --------------------------
# Check for dependencies
# --------------------------
echo "Checking dependencies..."
deps=(vulkaninfo zenity notify-send python3 tar)
missing=()
for dep in "${deps[@]}"; do
    if ! command -v $dep &>/dev/null; then
        missing+=($dep)
    fi
done

if [ ${#missing[@]} -ne 0 ]; then
    echo "Warning: missing dependencies: ${missing[*]}"
    echo "Please install them using your package manager, e.g.:"
    echo "Debian/Ubuntu: sudo apt install ${missing[*]}"
    echo "Fedora: sudo dnf install ${missing[*]}"
    echo "Arch: sudo pacman -S ${missing[*]}"
fi

# --------------------------
# Create local desktop shortcut
# --------------------------
DESKTOP_FILE="$HOME/.local/share/applications/pioza-launcher.desktop"
mkdir -p "$(dirname "$DESKTOP_FILE")"
cat > "$DESKTOP_FILE" <<EOL
[Desktop Entry]
Name=$APP_NAME
Exec=$APP_DIR/$LATEST_FILE
Icon=$ICON_FILE
Type=Application
Categories=Utility;
EOL

echo ""
echo "Installation complete."
echo "Run the app with:"
echo "$APP_DIR/$LATEST_FILE"
echo ""
echo "Or find it in your applications menu if your desktop environment supports ~/.local/share/applications."
