Name:           pioza-launcher
Version:        %{version}
Release:        1%{?dist}
Summary:        Pioza Launcher

License:        AGPLv3
URL:            https://github.com/Shieldowskyy/PiozaLauncher
Source0:        %{name}-%{version}.zip

BuildArch:      x86_64
Requires:       zenity, python3, tar, vulkan

%description
Pioza Launcher packaged as RPM.

%prep
# Unzip the source
unzip %{SOURCE0}

%install
mkdir -p %{buildroot}/opt/pioza-launcher
cp -r Linux %{buildroot}/opt/pioza-launcher/

mkdir -p %{buildroot}/usr/bin
ln -s /opt/pioza-launcher/Linux/PiozaGameLauncher.sh %{buildroot}/usr/bin/pioza-launcher

%files
/opt/pioza-launcher/Linux
/usr/bin/pioza-launcher

%changelog
* %{date} Shieldziak <shieldowskyy@proton.me> - %{version}-1
- Initial RPM release
