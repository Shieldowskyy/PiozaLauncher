# Pioza Launcher (formerly PiozaGameLauncher)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-%23313131.svg?logo=unrealengine&logoColor=white)](#)
[![Crowdin](https://badges.crowdin.net/piozagamelauncher/localized.svg)](https://crowdin.com/project/piozagamelauncher)
![image](https://github.com/user-attachments/assets/cfaa8d9f-5eab-43e9-a4b7-9b2d582fb933)



**Pioza Launcher** is an open-source launcher project created entirely in Unreal Engine 5 using blueprints. This makes it highly customizable for beginner programmers and indie game developers who want an easy and reliable way to distribute their games to players. It comes with several features, such as:

- Downloading, installing, and updating games  
- Moving files to other drives/folders  
- Displaying game descriptions, minimum requirements, screenshots, and videos  
- Playing sound effects when clicking on a game  
- Easily changing [repositories](https://github.com/Shieldowskyy/PiozaLauncher/wiki/Pioza-Repo-V2) via the console  
- Extending functionality with custom console libraries and more
- Creating backups of downloaded games  
- Making shortcut in start menu and on desktop
- Playtime tracking
- Linux system support
- [Offline Mode](https://github.com/Shieldowskyy/PiozaLauncher/wiki/Offline-Mode)
- Easy localization of game metadata  

### Planned Features:

- Verifying game files using checksums  
- Adding notes in the launcher menu for specific games  
- Support for changelogs based on HTML files
- Actions queue
- ARM64 platforms support
# Download

You can find the installer and zipped build in the [Releases](https://github.com/Shieldowskyy/PiozaGameLauncher/releases) tab.  
**Note:** The zipped build is **not portable** — it will save its configuration in the `LocalAppData` directory.

- **Windows:** Available both as an installer (`.exe`) and a zipped build. Winget install script is also available below.
- **Linux:** Available as .rpm and AppImage. Install script is also available below.


## Windows
### Support

Windows version supports all features and **does not require proton and dependencies** (which are shipped with release)

### 🖥️ Installation


#### Winget
Run this command in ``cmd`` or ``powershell`` (this may require admin privileges):

```bash
winget install "Pioza Launcher"
```
or if that doesn't work
```bash
curl -o pioza-launcher.yml https://raw.githubusercontent.com/Shieldowskyy/PiozaLauncher/main/pioza-launcher.yml
winget settings --enable LocalManifestFiles
winget install -m pioza-launcher.yml -h
```
## Linux

### Support

Linux support was introduced in **v0.6.8** and has been considered stable since **v0.7.1**.  
The launcher supports both **native Linux games** and **Windows games via Proton**.

### 🖥️ Installation

To install the latest version of Pioza Launcher on Linux, run this command:

```bash
curl -sSL dsh.yt/pioza.sh | bash
````

To uninstall it later, run:

```bash
curl -sSL dsh.yt/pioza.sh -o /tmp/pioza.sh
bash /tmp/pioza.sh --uninstall
```

### Dependencies

The Linux version of Pioza Launcher requires a few system dependencies:

- `vulkan` - required to run launcher gui
- `zenity` – used for file picker dialogs  
- `libnotify` - used for sending system notifications
- `python3` - used for running umu-launcher script
- `tar` - used for unpacking game backup archives

Install these using your distribution’s package manager.

# Opening the Project in Unreal Engine

Pioza Launcher is built with Unreal Engine. I found it easier to prototype some features using Unreal Blueprints, but I plan to rewrite parts of it in C++ in the future.

## Engine

To run this project, you must install Unreal Engine 5.5 on your machine. You might also need Visual Studio with the Unreal Engine components installed to compile all plugins and C++ code.

## Plugins

All required plugins are now included in the repository!

### Optional

> [!NOTE]  
> Although you might be able to run the project without these plugins, I recommend installing them to ensure the project starts without any issues.

Sentry is optional but recommended, and therefore included with the project:  
[Sentry](https://github.com/getsentry/sentry-unreal/releases)

# 📦 External Components

This project includes external components with their own licenses:

## Plugins
#### ✅ [RuntimeArchiver](https://github.com/gtreshchev/RuntimeArchiver)  
#### ✅ [RuntimeFilesDownloader](https://github.com/gtreshchev/RuntimeFilesDownloader)  
#### ✅ [RuntimeAudioImporter](https://github.com/gtreshchev/RuntimeAudioImporter)

All three are licensed under the **MIT License**.  
Copyright (c) 2024 Georgy Treshchev  
Full license texts are available in their respective repositories.

#### ✅ [Sentry Unreal](https://github.com/getsentry/sentry-unreal)

Licensed under the **MIT License**.  
Repository: https://github.com/getsentry/sentry-unreal

#### ✅ [DSHConsole](https://github.com/Shieldowskyy/DSHConsole)

Included as a Git submodule.  
Licensed under the **Apache License 2.0**.  
Repository: https://github.com/Shieldowskyy/DSHConsole

## External Runtimes
#### ✅ [UMU Launcher](https://github.com/Open-Wine-Components/umu-launcher)

Licensed under the **GNU General Public License v3.0 (GPLv3)**.  
**Not included** - downloaded automatically on first run only on Linux platform builds.  
Repository: https://github.com/Open-Wine-Components/umu-launcher

## Resources
#### ✅ [Material Icons by Google](https://fonts.google.com/icons)

Licensed under the **Apache License 2.0**.  
Available here: https://fonts.google.com/icons

---

### 📄 License

**From version 0.8.0 onwards**, this project is licensed under the **MIT License**.

This means you are free to:
- ✅ Use this code in commercial projects
- ✅ Create closed-source derivatives
- ✅ Modify and redistribute without restrictions
- ✅ Use in proprietary software

**Note:** Versions 0.7.x and earlier were licensed under AGPL-3.0. 
You can still use those versions under AGPL-3.0 terms if needed.

See [LICENSE](LICENSE) file for full terms.

Copyright © 2025 Shieldziak (DashoGames)
