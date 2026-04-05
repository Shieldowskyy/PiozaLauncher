
#!/usr/bin/env python3
import os
import sys
import subprocess

def register_windows():
    import winreg

    # Path to pioza_bootstrap.py or its compiled form
    # Usually you'd want to compile it to an EXE before registering
    # But for now we just register it with python.exe
    python_exe = sys.executable
    script_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "pioza_bootstrap.py"))

    command = f'"{python_exe}" "{script_path}" "%1"'

    try:
        # Create HKEY_CLASSES_ROOT\pioza
        key = winreg.CreateKey(winreg.HKEY_CLASSES_ROOT, "pioza")
        winreg.SetValue(key, "", winreg.REG_SZ, "URL:Pioza Protocol")
        winreg.SetValueEx(key, "URL Protocol", 0, winreg.REG_SZ, "")

        # Create HKEY_CLASSES_ROOT\pioza\shell\open\command
        shell_key = winreg.CreateKey(key, "shell\\open\\command")
        winreg.SetValue(shell_key, "", winreg.REG_SZ, command)

        print(f"Successfully registered pioza:// protocol on Windows.")
        print(f"Command: {command}")
    except Exception as e:
        print(f"Error registering on Windows: {e}")

def register_linux():
    # 1. Create a .desktop file
    current_dir = os.path.dirname(os.path.abspath(__file__))
    python_exe = sys.executable
    script_path = os.path.abspath(os.path.join(current_dir, "pioza_bootstrap.py"))

    desktop_content = f"""[Desktop Entry]
Name=Pioza Launcher Bootstrapper
Exec={python_exe} "{script_path}" "%U"
Type=Application
Terminal=false
NoDisplay=true
MimeType=x-scheme-handler/pioza;
"""

    home_dir = os.path.expanduser("~")
    applications_dir = os.path.join(home_dir, ".local", "share", "applications")
    os.makedirs(applications_dir, exist_ok=True)

    desktop_file = os.path.join(applications_dir, "pioza-launcher.desktop")
    with open(desktop_file, "w") as f:
        f.write(desktop_content)

    # 2. Update desktop database and register mime
    try:
        subprocess.run(["update-desktop-database", applications_dir], check=True)
        subprocess.run(["xdg-mime", "default", "pioza-launcher.desktop", "x-scheme-handler/pioza"], check=True)
        print(f"Successfully registered pioza:// protocol on Linux.")
        print(f"Created file: {desktop_file}")
    except Exception as e:
        print(f"Error registering on Linux: {e}")

def main():
    if sys.platform == "win32":
        register_windows()
    elif sys.platform == "linux":
        register_linux()
    else:
        print(f"Unsupported platform: {sys.platform}")

if __name__ == "__main__":
    main()
