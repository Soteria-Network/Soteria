
Debian
====================
This directory contains files used to package soteriad/soteria-qt
for Debian-based Linux systems. If you compile soteriad/soteria-qt yourself, there are some useful files here.

## soteria: URI support ##


soteria-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install soteria-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your soteria-qt binary to `/usr/bin`
and the `../../share/pixmaps/soteria128.png` to `/usr/share/pixmaps`

soteria-qt.protocol (KDE)

