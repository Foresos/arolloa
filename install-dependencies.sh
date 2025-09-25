#!/bin/bash
# install-dependencies.sh

echo "Installing Arolloa dependencies for Ubuntu..."

# Detect Ubuntu version
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_VERSION=$VERSION_ID
else
    echo "Cannot detect Ubuntu version"
    exit 1
fi

echo "Detected Ubuntu $OS_VERSION"

# Update package list
sudo apt update

# Essential build tools
sudo apt install -y build-essential git cmake ninja-build meson pkg-config

# Wayland core
sudo apt install -y wayland-protocols libwayland-dev wayland-scanner

# wlroots - version specific
case $OS_VERSION in
    "24.04"|"24.10")
        sudo apt install -y libwlroots-0.18-dev libwlroots-0.18
        ;;
    "22.04"|"22.10")
        sudo apt install -y libwlroots-dev
        ;;
    "20.04")
        sudo apt install -y libwlroots-dev
        ;;
    *)
        echo "Trying generic wlroots package..."
        sudo apt install -y libwlroots-dev || {
            echo "wlroots not available in repositories for this version"
            echo "You may need to build wlroots from source"
        }
        ;;
esac

# Graphics and rendering
sudo apt install -y \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libdrm-dev \
    libgbm-dev \
    libpixman-1-dev \
    libvulkan-dev

# Input and system
sudo apt install -y \
    libinput-dev \
    libxkbcommon-dev \
    libseat-dev \
    libsystemd-dev \
    libcap-dev

# X11 compatibility (recommended)
sudo apt install -y \
    libxcb-composite0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-render0-dev \
    libxcb-render-util0-dev \
    libxcb-xfixes0-dev \
    libx11-xcb-dev \
    libxcb-dri3-dev \
    libxcb-present-dev

# GUI libraries for applications
sudo apt install -y \
    libcairo2-dev \
    libpango1.0-dev \
    libgtk-3-dev \
    libjson-glib-1.0-dev

# Optional but useful
sudo apt install -y \
    libgudev-1.0-dev \
    libpng-dev \
    flatpak

echo "Dependencies installed! You can now run ./build.sh"
