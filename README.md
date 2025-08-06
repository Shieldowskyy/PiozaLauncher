# Pioza Launcher (formerly PiozaGameLauncher)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-%23313131.svg?logo=unrealengine&logoColor=white)](#)
[![Crowdin](https://badges.crowdin.net/piozagamelauncher/localized.svg)](https://crowdin.com/project/piozagamelauncher)
![image](https://github.com/user-attachments/assets/cfaa8d9f-5eab-43e9-a4b7-9b2d582fb933)



**Pioza Launcher** is an open-source launcher project created entirely in Unreal Engine 5 using blueprints. This makes it highly customizable for beginner programmers and indie game developers who want an easy and reliable way to distribute their games to players. It comes with several features, such as:

- Downloading, installing, and updating games  
- Moving files to other drives/folders  
- Displaying game descriptions, minimum requirements, screenshots, and videos  
- Playing sound effects when clicking on a game  
- Easily changing [repositories](https://github.com/Shieldowskyy/PiozaGameLauncher/wiki/PiozaRepo-(and-manifest)) via the console  
- Extending functionality with custom console libraries and more
- Creating backups of downloaded games  
- Making shortcut in start menu and on desktop
- Playtime tracking
- Linux system support
- [Offline Mode](https://github.com/Shieldowskyy/PiozaLauncher/wiki/Offline-Mode)

### Planned Features:
- Easy localization of game metadata  
- Merging [repositories ](https://github.com/Shieldowskyy/PiozaGameLauncher/wiki/PiozaRepo-(and-manifest)) 
- Verifying game files using checksums  
- Adding notes in the launcher menu for specific games  
- Support for changelogs based on HTML files
- Actions queue
# Download

You can find the installer and zipped build in the [Releases](https://github.com/Shieldowskyy/PiozaGameLauncher/releases) tab.  
**Note:** The zipped build is **not portable** — it will save its configuration in the `LocalAppData` directory.

- **Windows:** Available both as an installer (`.exe`) and a zipped build.
- **Linux:** Available only as an AppImage file.

## Linux

### Support

Linux support was introduced in **v0.6.8** and has been considered stable since **v0.7.1**.  
The launcher supports both **native Linux games** and **Windows games via Wine**.

### Dependencies

The Linux version of Pioza Launcher requires a few system dependencies:

- `zenity` – used for the file picker dialogs
- `wine` – required for running Windows games

Install these using your distribution’s package manager.
# Opening project in Unreal Engine
Pioza Launcher is built on Unreal Engine. It was easier for me to write something in Unreal Blueprints for simpler prototyping, but i have plans to rewrite some elements in C++ in future.
## Engine
To run this project, you have to install Unreal Engine 5.5 on your machine! Also you might need Visual Studio with configured Unreal Engine components to compile all plugins and C++ code!
## Plugins
### Optional
> [!NOTE]
> While you might already be able to run the project without these plugin, I recommend installing it so that project can start without any problems.

You can also install Sentry Unreal plugin to report all crashes to me. It will be helpful for investigating further problems!\
[Sentry](https://github.com/getsentry/sentry-unreal/releases)

# External Components

This project uses the `DSHConsole` component included as a Git submodule, which is licensed under the Apache License 2.0.

- DSHConsole repository: https://github.com/Shieldowskyy/DSHConsole
- License: Apache License 2.0

The rest of this project is licensed under the Affero General Public License version 3 (AGPLv3).
