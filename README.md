# PiozaGameLauncher
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-%23313131.svg?logo=unrealengine&logoColor=white)](#)
[![Crowdin](https://badges.crowdin.net/piozagamelauncher/localized.svg)](https://crowdin.com/project/piozagamelauncher)

An open source game launcher for Dasho Games titles and more!

# Download
In Releases tab you will find installer and zipped build that you can install where you want.\
NOTE: Zipped build is not portable and will save config in localappdata directory.\

 Currently only Windows builds are available but there are some plans to port it to Linux and run our games via Proton!

# Opening project in Unreal Engine
PiozaGameLauncher is built on Unreal Engine. It was easier for me to write something in Unreal Blueprints for simpler prototyping, but i have plans to rewrite some elements in C++ in future.
## Engine
To run this project, you have to install Unreal Engine 5.3 on your machine! Also you might need Visual Studio with configured Unreal Engine components to compile all plugins and C++ code!
## Plugins
To open project you need to install these plugins:\
[EasyFileDialog](https://github.com/unrealsumon/EasyFileDialog) from Github or Marketplace\
and\
[File Helper Blueprint Library](https://www.unrealengine.com/marketplace/en-US/product/file-helper-bp-library) from Marketplace
### Optional
> [!NOTE]
> While you might already be able to run the project without these plugin, I recommend installing it so that project can start without any problems.

You can also install Sentry Unreal plugin to report all crashes to me. It will be helpful for investigating further problems!\
[Sentry](https://github.com/getsentry/sentry-unreal/releases)
