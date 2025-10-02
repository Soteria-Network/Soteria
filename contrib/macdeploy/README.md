### MacDeploy ###

For macOS Mountain Lion, you will need the param_parser package:
	
	sudo easy_install argparse

This script should not be run manually, instead, after building as usual:

	make deploy

During the process, the disk image window will pop up briefly where the fancy
settings are applied. This is normal, please do not interfere.

When finished, it will produce `Soteria-Core.dmg`.

