fmstick_ui
==========

User-Interface application for FmStick USB FM Transmitter


Supported features:
* RadioText Plus
* fetching tadiotext from file
* parsing radiotext and set RT+ tags automatically
* HTTP web-interface with HTTP-api.

Install (Debian 12/13, Raspberry Pi OS)
---------------------------------------

Download the `.deb` for your system from the
[releases](https://github.com/DABodr/fmstick/releases): `+deb13` for
Debian 13 trixie (current Raspberry Pi OS), `+deb12` for Debian 12
bookworm; `amd64` for a PC, `arm64` for a 64-bit Raspberry Pi. Then
install it with apt, which fetches the Qt6 libraries:

    sudo apt install ./fmstick_2.0+deb13_arm64.deb

Plug the transmitter in afterwards: the package's udev rule lets users
drive it without root.

Build
-----

Qt6, libusb-1.0 and ALSA development packages, then, out of tree:

    mkdir build && cd build && /usr/lib/qt6/bin/qmake ../fmstick.pro && make -j4

Or a Debian package: `dpkg-buildpackage -b -us -uc`.
