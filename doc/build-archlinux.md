Arch Linux build guide
----------------------

This example lists the steps necessary to setup and build a command line only
dingocoind on archlinux:

```sh
pacman -S git base-devel boost libevent python3 db
git clone https://github.com/soteria-network/soteria.git
cd soteria/
./autogen.sh
./configure --without-gui --without-miniupnpc
make
```
