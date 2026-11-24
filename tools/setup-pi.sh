#!/usr/bin/env bash
# One-time setup of the Raspberry Pi 3 for the rpm project (Raspberry Pi OS Lite).
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"

echo "==> Installing packages"
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git pkg-config \
    gcc-avr avr-libc avrdude \
    picocom pipx gdb

echo "==> Installing Conan 2"
pipx ensurepath
pipx install "conan>=2.0" || pipx upgrade conan
export PATH="$HOME/.local/bin:$PATH"
conan profile detect --exist-ok
# The project is C++20; the detected profile defaults to gnu17.
sed -i 's/^compiler.cppstd=.*/compiler.cppstd=20/' "$(conan profile path default)"

echo "==> Serial port access (dialout group)"
sudo usermod -aG dialout "$USER"

echo "==> udev rule for /dev/mega"
sudo cp "$here/udev/99-rpm-mega.rules" /etc/udev/rules.d/
sudo udevadm control --reload
sudo udevadm trigger

echo
echo "Done. Log out and back in (dialout group), then plug in the Mega and check: ls -l /dev/mega"
