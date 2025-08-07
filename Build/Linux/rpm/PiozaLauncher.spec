Name:           pioza-launcher
Version:        %{version}
Release:        1%{?dist}
Summary:        Pioza Launcher

License:        AGPLv3
URL:            https://github.com/Shieldowskyy/PiozaLauncher
Source0:        %{name}-%{version}.zip
Source1:        PiozaGameLauncher.desktop
Source2:        pioza_icon.png

BuildArch:      x86_64
Requires:       zenity, python3, tar, vulkan, libnotify

%description
Pioza Launcher packaged as RPM.

%prep
# Unzip the main source archive
unzip %{SOURCE0}

%install
# Install launcher files
mkdir -p %{buildroot}/opt/pioza-launcher
cp -r Linux %{buildroot}/opt/pioza-launcher/

# Create symlink to main script
mkdir -p %{buildroot}/usr/bin
ln -s /opt/pioza-launcher/Linux/PiozaGameLauncher.sh %{buildroot}/usr/bin/pioza-launcher

# Install .desktop file
mkdir -p %{buildroot}/usr/share/applications
cp %{SOURCE1} %{buildroot}/usr/share/applications/pioza-launcher.desktop

# Install icon
mkdir -p %{buildroot}/usr/share/icons/hicolor/128x128/apps
cp %{SOURCE2} %{buildroot}/usr/share/icons/hicolor/128x128/apps/pioza_icon.png

%files
/opt/pioza-launcher/Linux
/usr/bin/pioza-launcher
/usr/share/applications/pioza-launcher.desktop
/usr/share/icons/hicolor/128x128/apps/pioza_icon.png

%changelog
* %{date} Shieldziak <shieldowskyy@proton.me> - %{version}-1
- Initial RPM release with desktop integration
