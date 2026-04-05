
#!/usr/bin/env python3
import sys
import os
import socket
import subprocess
import time
import re

PORT = 55562
LAUNCHER_EXE_NAME = "PiozaGameLauncher"

def get_launcher_path():
    """Find the launcher executable relative to this script."""
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # Assuming packaged structure: Tools/bootstrapper/pioza_bootstrap.py
    # Launcher should be in the root or Binaries folder

    root_dir = os.path.abspath(os.path.join(current_dir, "..", ".."))

    if sys.platform == "win32":
        # 1. Check for binary in root
        exe_path = os.path.join(root_dir, f"{LAUNCHER_EXE_NAME}.exe")
        if not os.path.isfile(exe_path):
            # 2. Check in Binaries/Win64 (Direct)
            exe_path = os.path.join(root_dir, "Binaries", "Win64", f"{LAUNCHER_EXE_NAME}.exe")
            if not os.path.isfile(exe_path):
                # 3. Check in Binaries/Win64 (Shipping)
                exe_path = os.path.join(root_dir, "Binaries", "Win64", f"{LAUNCHER_EXE_NAME}-Win64-Shipping.exe")
                if not os.path.isfile(exe_path):
                    # 4. Fallback to standard UE packaged location
                    exe_path = os.path.join(root_dir, LAUNCHER_EXE_NAME, "Binaries", "Win64", f"{LAUNCHER_EXE_NAME}-Win64-Shipping.exe")
    else:
        # 1. Check for binary in root
        exe_path = os.path.join(root_dir, LAUNCHER_EXE_NAME)
        if not os.path.isfile(exe_path):
            # 2. Check for AppImage
            appimages = [f for f in os.listdir(root_dir) if f.endswith(".AppImage")]
            if appimages:
                exe_path = os.path.join(root_dir, appimages[0])
            else:
                # 3. Check for Linux binary in Binaries (without extra folder)
                exe_path = os.path.join(root_dir, "Binaries", "Linux", f"{LAUNCHER_EXE_NAME}-Linux-Shipping")
                if not os.path.isfile(exe_path):
                    # 4. Fallback to standard UE packaged location
                    exe_path = os.path.join(root_dir, LAUNCHER_EXE_NAME, "Binaries", "Linux", f"{LAUNCHER_EXE_NAME}-Linux-Shipping")

    return exe_path

def parse_protocol(url):
    """Extract game ID from pioza://start-game/gameid"""
    match = re.match(r"pioza://start-game/(.+)", url)
    if match:
        return match.group(1).rstrip('/')
    return None

def send_to_launcher(game_id):
    """Try to send the game ID to the running launcher."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(2)
            s.connect(("127.0.0.1", PORT))
            s.sendall(game_id.encode('utf-8'))
            return True
    except ConnectionRefusedError:
        return False
    except Exception as e:
        print(f"Error sending to launcher: {e}")
        return False

def main():
    game_id = None

    if len(sys.argv) > 1:
        protocol_url = sys.argv[1]
        # Guard against .desktop placeholder not being substituted (no URL passed)
        if protocol_url in ("%u", "%U", ""):
            protocol_url = None

        if protocol_url:
            parsed_id = parse_protocol(protocol_url)
            if parsed_id:
                game_id = parsed_id
            else:
                # Maybe it's just the game ID passed directly
                game_id = protocol_url

    # 1. Try to send to already running instance
    if send_to_launcher(game_id or "default"):
        print(f"Sent launch command for '{game_id}' to running launcher.")
        sys.exit(0)

    # 2. Launcher not running, start it
    launcher_path = get_launcher_path()
    if not os.path.exists(launcher_path):
        print(f"Error: Launcher executable not found at {launcher_path}")
        sys.exit(1)

    # Build args - only pass -start-game if we have an actual game ID
    launch_args = [launcher_path]
    if game_id:
        launch_args.append(f"-start-game={game_id}")
        print(f"Executing: {launcher_path} -start-game={game_id}")
    else:
        print(f"Executing: {launcher_path} (no game ID)")

    print(f"Starting launcher: {launcher_path}")
    # Start launcher in background
    try:
        if sys.platform == "win32":
            subprocess.Popen(launch_args, creationflags=subprocess.CREATE_NEW_CONSOLE)
        else:
            subprocess.Popen(launch_args, start_new_session=True)
    except Exception as e:
        print(f"CRITICAL ERROR: Failed to launch process: {e}")
        sys.exit(1)

    # 3. Wait for launcher to initialize its TCP server
    print("Waiting for launcher to initialize...")
    max_attempts = 30
    for i in range(max_attempts):
        time.sleep(0.5)
        if send_to_launcher(game_id or "default"):
            print(f"Launcher ready. Sent launch command for '{game_id or 'default'}'.")
            sys.exit(0)

    print("Error: Timed out waiting for launcher to become ready.")
    sys.exit(1)

if __name__ == "__main__":
    main()
