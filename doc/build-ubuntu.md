Build instructions for Soteria 
=================================

This will install most of the dependencies from ubuntu.
The only one we build, is Berkeley DB 4.8.

Ubuntu 24.04/10/25.04 - Noble Numbat/Oracular Oriole/Plucky Puffin - Plus Berkeley DB 4.8.30 Script - Install dependencies:
---------------------------
$ sudo apt install 
build-essential
autoconf 
automake 
autotools-dev 
libtool 
pkg-config 
bsdmainutils 
python3 
patch 
make 
cmake 
g++ 
gcc 
binutils 
binutils-gold 
libboost-all-dev 
libssl-dev 
libevent-dev 
libczmq-dev 
libminiupnpc-dev 
libprotobuf-dev 
libqrencode-dev 
zlib1g-dev 
qtbase5-dev 
qttools5-dev 
qttools5-dev-tools 
libexpat1-dev 
libdbus-1-dev 
libfontconfig-dev 
libfreetype-dev 
libice-dev 
libsm-dev 
libx11-dev 
libxau-dev 
libxext-dev 
libxcb1-dev 
libxkbcommon-dev 
xcb-proto 
x11proto-xext-dev 
x11proto-dev 
xtrans-dev 
bison 
protobuf-compiler 
curl 
wget 
g++-mingw-w64-x86-64 
mingw-w64-x86-64-dev 
nsis 
doxygen 
graphviz 
gettext 
ccache 
git 
btop 
zip 
unzip 

Ubuntu 22.04 - Jammy Jellyfish - Install dependencies:
---------------------------
`$ sudo apt install 
build-essential
libssl-dev
libboost-chrono-dev
libboost-filesystem-dev
libboost-program-options-dev
libboost-system-dev
libboost-thread-dev
libboost-test-dev
qtbase5-dev
qttools5-dev
bison
libexpat1-dev
libdbus-1-dev
libfontconfig-dev
libfreetype6-dev
libice-dev
libsm-dev
libx11-dev
libxau-dev
libxext-dev
libevent-dev
libxcb1-dev
libxkbcommon-dev
libminiupnpc-dev
libprotobuf-dev
libqrencode-dev
xcb-proto
x11proto-xext-dev
x11proto-dev
xtrans-dev
zlib1g-dev
libczmq-dev
autoconf
automake
libtool
protobuf-compiler
libdb++-dev

Ubuntu 21.04 - Hirsute Hippo - Install dependencies:
----------------------------
`$ sudo apt install 
build-essential
libssl-dev
libboost-chrono1.71-dev
libboost-filesystem1.71-dev
libboost-program-options1.71-dev
libboost-system1.71-dev
libboost-thread1.71-dev
libboost-test1.71-dev
qtbase5-dev
qttools5-dev
bison
libexpat1-dev
libdbus-1-dev
libfontconfig-dev
libfreetype-dev
libice-dev
libsm-dev
libx11-dev
libxau-dev
libxext-dev
libevent-dev
libxcb1-dev
libxkbcommon-dev
libminiupnpc-dev
libprotobuf-dev
libqrencode-dev
xcb-proto
x11proto-xext-dev
x11proto-dev
xtrans-dev
zlib1g-dev
libczmq-dev
autoconf
automake
libtool
protobuf-compiler
libdb++-dev`

Ubuntu 18.04 - Bionic Beaver - Install dependencies:
----------------------------
`$ sudo apt install 
build-essential
libssl-dev
libboost-chrono-dev
libboost-filesystem-dev
libboost-program-options-dev
libboost-system-dev
libboost-thread-dev
libboost-test-dev
qtbase5-dev
qttools5-dev
bison
libexpat1-dev
libdbus-1-dev
libfontconfig-dev
libfreetype6-dev
libice-dev
libsm-dev
libx11-dev
libxau-dev
libxext-dev
libevent-dev
libxcb1-dev
libxkbcommon-dev
libminiupnpc-dev
libprotobuf-dev
libqrencode-dev
xcb-proto
x11proto-xext-dev
x11proto-dev
xtrans-dev
zlib1g-dev
libczmq-dev
autoconf
automake
libtool
protobuf-compiler
`

Directory structure
------------------
Soteria sources in `$HOME/src`

Berkeley DB will be installed to `$HOME/src/db4`


Soteria
------------------

Start in $HOME

Make the directory for sources and go into it.

`mkdir src`

`cd src`

__Download Soteria source.__

`git clone https://github.com/Soteria-Network/Soteria`

`cd Soteria`

`git checkout develop` # this checks out the develop branch.

__Download and build Berkeley DB 4.8__

`bash contrib/install_db4.sh ../`

__The build process:__

`./autogen.sh`

`export BDB_PREFIX=$HOME/src/db4`

`./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" --prefix=/usr/local` 

_Adjust to own needs. This will install the binaries to `/usr/local/bin`_


`make -j4`  # 4 for 4 build threads, adjust to fit your setup.

You can now start soteria-qt from the build directory.

`src/qt/soteria-qt`

soteriad and soteria-cli are in `src/`


__Optional:__

`sudo make install`  # if you want to install the binaries to /usr/local/bin (if this prefix was used above).

Only Daemon:

mkdir src

cd src

git clone https://github.com/NeuraiProject/Neurai

cd Neurai

contrib/install_db4.sh ../

./autogen.sh

export BDB_PREFIX=$HOME/src/db4

./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" --prefix=/usr/local --with-gui=no --with-miniupnpc=no

make -j4

__Optional:__

sudo make install  # if you want to install the binaries to /usr/local/bin
