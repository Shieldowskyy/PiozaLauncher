# DSHConsole
A simple customizable DebugConsole for Unreal Engine! Made (almost) entirely in Blueprints!
![image-removebg-preview](https://github.com/user-attachments/assets/5d662856-8055-4734-839b-fcc63878aa2b)

This plugin was made for internal use in DashoGames and also for PiozaLauncher project, for easier debugging and testing.


### WARNING
I currently do **NOT** provide any support or docs for this plugin.
This repository is public to facilitate the development of PiozaLauncher via a separate submodule.

# Compatibility
Current version is compatibile and tested with Unreal Engine 5.5.0 on Windows and Linux

# Implementation Tips
## Spawning Widget
The console should be created *as early as possible* in the game's lifecycle. In Blueprints, the best place for this is the **GameInstance**.

You should use the **'Create Widget (W_DebugConsole)'** node right after the **Event Init**, and then store the widget in a **variable**. This allows logging to the console from the very beginning and enables command execution at any point in the future.

Additionally, it's important to ensure that whenever the console is shown on the player's screen, the **mouse cursor is enabled** so the player can focus on the console's text input field.
