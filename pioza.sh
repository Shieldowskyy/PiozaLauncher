#!/usr/bin/env bash
set -e

# Colors (bright/bold variants)
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
MAGENTA='\033[1;35m'
NC='\033[0m' # No Color

APP_NAME="PiozaLauncher"
APP_DIR="$HOME/.pioza-launcher"
ICON_URL="https://github.com/Shieldowskyy/PiozaLauncher/raw/main/pioza.ico"
GITHUB_API="https://api.github.com/repos/Shieldowskyy/PiozaLauncher/releases/latest"
DESKTOP_FILE="$HOME/.local/share/applications/pioza-launcher.desktop"
ICON_FILE="$APP_DIR/pioza.png"
BIN_LINK="$HOME/.local/bin/pioza-launcher"

show_help() {
    echo -e "${MAGENTA}╔══════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║  ${CYAN}${APP_NAME} installer script${MAGENTA}      ║${NC}"
    echo -e "${MAGENTA}╚══════════════════════════════════════╝${NC}"
    echo
    echo -e "${BLUE}Usage:${NC}"
    echo -e "  ${CYAN}$(basename "$0")${NC}            Install or update ${APP_NAME}"
    echo -e "  ${CYAN}$(basename "$0") --uninstall${NC}  Remove ${APP_NAME} completely"
    echo -e "  ${CYAN}$(basename "$0") --help${NC}       Show this help message"
}


uninstall() {
    echo -e "${YELLOW}Uninstalling $APP_NAME...${NC}"
    rm -f "$DESKTOP_FILE"
    rm -f "$BIN_LINK"
    rm -rf "$APP_DIR"
    echo -e "${GREEN}✓ Uninstallation complete.${NC}"
    exit 0
}

# --------------------------
# Check system architecture
# --------------------------
check_architecture() {
    ARCH=$(uname -m)
    echo -e "${BLUE}Detected architecture: ${CYAN}$ARCH${NC}"
    
    case "$ARCH" in
        x86_64|amd64)
            echo -e "${GREEN}✓ x86_64 architecture supported${NC}"
            ;;
        aarch64|arm64)
            echo -e "${GREEN}✓ ARM64 architecture detected${NC}"
            ;;
        i386|i686)
            echo -e "${RED}✗ Error: 32-bit architecture not supported${NC}"
            exit 1
            ;;
        *)
            echo -e "${YELLOW}⚠ Warning: Unsupported architecture: $ARCH${NC}"
            read -p "Continue anyway? (y/n) " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
}

# --------------------------
# Check Vulkan support
# --------------------------
check_vulkan_support() {
    echo -e "${BLUE}Checking Vulkan support...${NC}"
    
    # Check if vulkaninfo is available
    if ! command -v vulkaninfo &>/dev/null; then
        echo -e "${YELLOW}⚠ Warning: vulkaninfo not found, cannot verify Vulkan support${NC}"
        echo -e "${YELLOW}  The launcher may not work without Vulkan-capable GPU${NC}"
        return 0
    fi
    
    # Run vulkaninfo and check for devices
    local vulkan_output
    vulkan_output=$(vulkaninfo --summary 2>/dev/null)
    
    if [ -z "$vulkan_output" ]; then
        echo -e "${RED}✗ Error: No Vulkan support detected!${NC}"
        echo -e "${RED}  This launcher requires a Vulkan-capable graphics card.${NC}"
        echo ""
        echo -e "${YELLOW}Possible solutions:${NC}"
        echo -e "${YELLOW}  1. Update your GPU drivers${NC}"
        echo -e "${YELLOW}  2. Install vulkan drivers for your GPU:${NC}"
        
        if command -v dnf &>/dev/null; then
            echo -e "${CYAN}     Fedora: sudo dnf install mesa-vulkan-drivers vulkan${NC}"
        elif command -v apt &>/dev/null; then
            echo -e "${CYAN}     Debian/Ubuntu: sudo apt install mesa-vulkan-drivers vulkan-tools${NC}"
        elif command -v pacman &>/dev/null; then
            echo -e "${CYAN}     Arch: sudo pacman -S vulkan-icd-loader lib32-vulkan-icd-loader${NC}"
        elif command -v zypper &>/dev/null; then
            echo -e "${CYAN}     openSUSE: sudo zypper install libvulkan1 vulkan-tools${NC}"
        fi
        
        echo ""
        read -p "Continue installation anyway? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
        return 1
    fi
    
    # Extract GPU info
    local gpu_name
    local api_version
    gpu_name=$(echo "$vulkan_output" | grep "deviceName" | head -1 | sed 's/.*= //')
    api_version=$(echo "$vulkan_output" | grep "apiVersion" | head -1 | sed 's/.*= //')
    
    if [ -n "$gpu_name" ]; then
        echo -e "${GREEN}✓ Vulkan support detected${NC}"
        echo -e "${CYAN}  GPU: ${gpu_name}${NC}"
        [ -n "$api_version" ] && echo -e "${CYAN}  Vulkan API: ${api_version}${NC}"
    else
        echo -e "${YELLOW}⚠ Vulkan installation found but GPU info unavailable${NC}"
    fi
    
    return 0
}

# --------------------------
# Check available disk space
# --------------------------
check_disk_space() {
    local required_bytes=$1
    local required_mb=$((required_bytes / 1024 / 1024))
    
    local available_kb
    available_kb=$(df "$APP_DIR" 2>/dev/null | tail -1 | awk '{print $4}')
    
    if [ -z "$available_kb" ]; then
        echo -e "${YELLOW}⚠ Warning: Could not check disk space${NC}"
        return 0
    fi
    
    local available_mb=$((available_kb / 1024))
    
    echo -e "${BLUE}Required space: ${CYAN}${required_mb} MB${NC}"
    echo -e "${BLUE}Available space: ${CYAN}${available_mb} MB${NC}"
    
    if [ "$available_mb" -lt "$required_mb" ]; then
        echo -e "${RED}✗ Error: Not enough disk space!${NC}"
        echo -e "${RED}  Required: ${required_mb} MB${NC}"
        echo -e "${RED}  Available: ${available_mb} MB${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Sufficient disk space available${NC}"
}

# --------------------------
# Verify downloaded file
# --------------------------
verify_download() {
    local file_path=$1
    
    if [ ! -f "$file_path" ]; then
        echo -e "${RED}✗ Error: Downloaded file does not exist${NC}"
        return 1
    fi
    
    if [ ! -s "$file_path" ]; then
        echo -e "${RED}✗ Error: Downloaded file is empty${NC}"
        rm -f "$file_path"
        return 1
    fi
    
    local actual_size
    local actual_size_mb
    actual_size=$(stat -c%s "$file_path" 2>/dev/null || stat -f%z "$file_path" 2>/dev/null)
    actual_size_mb=$((actual_size / 1024 / 1024))
    
    echo -e "${BLUE}Downloaded file size: ${CYAN}${actual_size_mb} MB${NC}"
    
    # Check if file is executable
    if ! file "$file_path" | grep -q "executable"; then
        echo -e "${YELLOW}⚠ Warning: Downloaded file may not be a valid executable${NC}"
    fi
    
    echo -e "${GREEN}✓ Download verified successfully${NC}"
    return 0
}

# --------------------------
# Detect package manager and install dependencies
# --------------------------
install_dependencies() {
    local missing=("$@")
    
    if [ ${#missing[@]} -eq 0 ]; then
        return 0
    fi
    
    echo -e "${YELLOW}⚠ Missing dependencies: ${missing[*]}${NC}"
    echo -e "${BLUE}Attempting to install automatically...${NC}"
    
    # Detect package manager
    if command -v dnf &>/dev/null; then
        # Fedora/RHEL
        echo -e "${CYAN}Using dnf package manager...${NC}"
        
        # Map package names for Fedora
        local fedora_packages=()
        for dep in "${missing[@]}"; do
            case "$dep" in
                vulkaninfo) fedora_packages+=("vulkan-tools") ;;
                notify-send) fedora_packages+=("libnotify") ;;
                *) fedora_packages+=("$dep") ;;
            esac
        done
        
        if sudo dnf install -y "${fedora_packages[@]}" 2>/dev/null; then
            echo -e "${GREEN}✓ Dependencies installed successfully${NC}"
        else
            echo -e "${YELLOW}⚠ Some dependencies may not have been installed${NC}"
        fi
        
    elif command -v apt &>/dev/null; then
        # Debian/Ubuntu
        echo -e "${CYAN}Using apt package manager...${NC}"
        
        local apt_packages=()
        for dep in "${missing[@]}"; do
            case "$dep" in
                vulkaninfo) apt_packages+=("vulkan-tools") ;;
                notify-send) apt_packages+=("libnotify-bin") ;;
                *) apt_packages+=("$dep") ;;
            esac
        done
        
        sudo apt update -qq 2>/dev/null
        if sudo apt install -y "${apt_packages[@]}" 2>/dev/null; then
            echo -e "${GREEN}✓ Dependencies installed successfully${NC}"
        else
            echo -e "${YELLOW}⚠ Some dependencies may not have been installed${NC}"
        fi
        
    elif command -v pacman &>/dev/null; then
        # Arch Linux
        echo -e "${CYAN}Using pacman package manager...${NC}"
        
        local arch_packages=()
        for dep in "${missing[@]}"; do
            case "$dep" in
                vulkaninfo) arch_packages+=("vulkan-tools") ;;
                notify-send) arch_packages+=("libnotify") ;;
                *) arch_packages+=("$dep") ;;
            esac
        done
        
        if sudo pacman -S --noconfirm "${arch_packages[@]}" 2>/dev/null; then
            echo -e "${GREEN}✓ Dependencies installed successfully${NC}"
        else
            echo -e "${YELLOW}⚠ Some dependencies may not have been installed${NC}"
        fi
        
    elif command -v zypper &>/dev/null; then
        # openSUSE
        echo -e "${CYAN}Using zypper package manager...${NC}"
        
        local suse_packages=()
        for dep in "${missing[@]}"; do
            case "$dep" in
                vulkaninfo) suse_packages+=("vulkan-tools") ;;
                notify-send) suse_packages+=("libnotify-tools") ;;
                *) suse_packages+=("$dep") ;;
            esac
        done
        
        if sudo zypper install -y "${suse_packages[@]}" 2>/dev/null; then
            echo -e "${GREEN}✓ Dependencies installed successfully${NC}"
        else
            echo -e "${YELLOW}⚠ Some dependencies may not have been installed${NC}"
        fi
        
    else
        echo -e "${RED}✗ Unknown package manager. Please install manually:${NC}"
        echo -e "${YELLOW}${missing[*]}${NC}"
        return 1
    fi
}

# --------------------------
# Argument handling
# --------------------------
case "$1" in
    --help|-h)
        show_help
        exit 0
        ;;
    --uninstall)
        uninstall
        ;;
    "" ) ;; # no arguments -> continue install
    * )
        echo -e "${RED}Unknown argument: $1${NC}"
        show_help
        exit 1
        ;;
esac

# --------------------------
# Pre-flight checks
# --------------------------
echo -e "${CYAN}=== Pre-flight checks ===${NC}"
check_architecture
echo ""
check_vulkan_support

echo ""
echo -e "${CYAN}=== Checking latest release of $APP_NAME ===${NC}"

# --------------------------
# Create target directory
# --------------------------
mkdir -p "$APP_DIR"
mkdir -p "$HOME/.local/bin"

# --------------------------
# Get latest AppImage info
# --------------------------
echo -e "${BLUE}Fetching release information...${NC}"
RELEASE_DATA=$(curl -s $GITHUB_API)

if [ -z "$RELEASE_DATA" ]; then
    echo -e "${RED}✗ Error: Could not fetch release information${NC}"
    exit 1
fi

LATEST_URL=$(echo "$RELEASE_DATA" | grep "browser_download_url.*AppImage" | cut -d '"' -f 4)
LATEST_FILE=$(basename "$LATEST_URL")

if [ -z "$LATEST_URL" ]; then
    echo -e "${RED}✗ Error: Could not find AppImage download URL${NC}"
    exit 1
fi

echo -e "${GREEN}Latest version: ${CYAN}$LATEST_FILE${NC}"

# Require 1GB free space for installation
REQUIRED_SPACE_MB=1024
echo -e "${BLUE}Checking disk space (requires ${REQUIRED_SPACE_MB}MB free)...${NC}"
check_disk_space $((REQUIRED_SPACE_MB * 1024 * 1024))

# --------------------------
# Download or skip if exists
# --------------------------
if [ -f "$APP_DIR/$LATEST_FILE" ]; then
    echo -e "${GREEN}✓ You already have the latest version: $LATEST_FILE${NC}"
else
    echo ""
    echo -e "${CYAN}=== Downloading latest version ===${NC}"
    echo -e "${BLUE}From: ${CYAN}$LATEST_URL${NC}"
    
    if ! curl -L --progress-bar "$LATEST_URL" -o "$APP_DIR/$LATEST_FILE"; then
        echo -e "${RED}✗ Error: Download failed${NC}"
        rm -f "$APP_DIR/$LATEST_FILE"
        exit 1
    fi
    
    echo ""
    echo -e "${CYAN}=== Verifying download ===${NC}"
    if ! verify_download "$APP_DIR/$LATEST_FILE"; then
        echo -e "${RED}✗ Installation failed: Download verification failed${NC}"
        exit 1
    fi
    
    chmod +x "$APP_DIR/$LATEST_FILE"
    
    # Remove old versions
    echo -e "${BLUE}Cleaning up old versions...${NC}"
    for f in "$APP_DIR"/*.AppImage; do
        if [ "$f" != "$APP_DIR/$LATEST_FILE" ]; then
            echo -e "${YELLOW}Removing: $(basename "$f")${NC}"
            rm "$f"
        fi
    done
fi

# --------------------------
# Create symlink for CLI access
# --------------------------
echo ""
echo -e "${BLUE}Creating command-line shortcut...${NC}"
ln -sf "$APP_DIR/$LATEST_FILE" "$BIN_LINK"

# --------------------------
# Download icon and convert to PNG
# --------------------------
if [ ! -f "$ICON_FILE" ]; then
    echo -e "${BLUE}Downloading icon...${NC}"
    curl -sL "$ICON_URL" -o "$APP_DIR/pioza.ico"
    
    # Convert .ico to .png (use magick instead of deprecated convert)
    if command -v magick &>/dev/null; then
        magick "$APP_DIR/pioza.ico" "$ICON_FILE" 2>/dev/null
    elif command -v convert &>/dev/null; then
        convert "$APP_DIR/pioza.ico" "$ICON_FILE" 2>/dev/null
    else
        echo -e "${YELLOW}⚠ Warning: ImageMagick not found, cannot convert icon${NC}"
        echo -e "${YELLOW}  Install 'imagemagick' or rename .ico to .png manually${NC}"
        ICON_FILE="$APP_DIR/pioza.ico"
    fi
fi

# --------------------------
# Check for dependencies
# --------------------------
echo ""
echo -e "${CYAN}=== Checking dependencies ===${NC}"
deps=(vulkaninfo zenity notify-send python3 tar)
missing=()

for dep in "${deps[@]}"; do
    if ! command -v "$dep" &>/dev/null; then
        missing+=("$dep")
    fi
done

if [ ${#missing[@]} -ne 0 ]; then
    install_dependencies "${missing[@]}"
else
    echo -e "${GREEN}✓ All dependencies are installed${NC}"
fi

# --------------------------
# Create local desktop shortcut
# --------------------------
mkdir -p "$(dirname "$DESKTOP_FILE")"
cat > "$DESKTOP_FILE" <<EOL
[Desktop Entry]
Name=$APP_NAME
Exec=$APP_DIR/$LATEST_FILE
Icon=$ICON_FILE
Type=Application
Categories=Utility;
EOL

# --------------------------
# Check if ~/.local/bin is in PATH
# --------------------------
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo ""
    echo -e "${YELLOW}⚠  Warning: $HOME/.local/bin is not in your PATH${NC}"
    echo -e "${BLUE}Add this line to your ~/.bashrc or ~/.zshrc:${NC}"
    echo ""
    echo -e "${CYAN}export PATH=\"\$HOME/.local/bin:\$PATH\"${NC}"
    echo ""
    echo -e "${BLUE}Then reload your shell with: ${CYAN}source ~/.bashrc${NC}"
fi

echo ""
echo -e "${GREEN}===================================${NC}"
echo -e "${GREEN}✓ Installation complete!${NC}"
echo -e "${GREEN}===================================${NC}"
echo -e "${BLUE}Run the app with:${NC}"
echo -e "${CYAN}  pioza-launcher${NC}         (from anywhere in terminal)"
echo -e "${CYAN}  $APP_DIR/$LATEST_FILE${NC}  (direct path)"
echo ""
echo -e "${BLUE}Or find it in your applications menu.${NC}"
