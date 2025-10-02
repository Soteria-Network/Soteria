#!/usr/bin/env bash
# Set DISTNAME, BRANCH and MAKEOPTS to the desired settings
DISTNAME=soteria-1.0.0
MAKEOPTS="-j4"
BRANCH=master
clear
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run with sudo"
   exit 1
fi
if [[ $PWD != $HOME ]]; then
   echo "This script must be run from ~/"
   exit 1
fi
if [ ! -f ~/MacOSX11.3.sdk.tar.gz ]
then
	echo "Before executing script.sh transfer MacOSX11.3.sdk.tar.gz to ~/"
	exit 1
fi
export PATH_orig=$PATH

echo @@@
echo @@@"Installing Dependecies"
echo @@@

apt install -y curl g++-aarch64-linux-gnu g++-9-aarch64-linux-gnu gcc-9-aarch64-linux-gnu binutils-aarch64-linux-gnu g++-arm-linux-gnueabihf g++-9-arm-linux-gnueabihf gcc-9-arm-linux-gnueabihf binutils-arm-linux-gnueabihf g++-9-multilib gcc-9-multilib binutils-gold git pkg-config autoconf libtool automake bsdmainutils ca-certificates python g++ mingw-w64 g++-mingw-w64 nsis zip rename librsvg2-bin libtiff-tools cmake imagemagick libcap-dev libz-dev libbz2-dev python-dev python-setuptools fonts-tuffy
cd ~/

# Removes any existing builds and starts clean WARNING
rm -rf ~/soteria ~/sign ~/release

git clone https://github.com/soterianetwork/soteria
cd ~/soteria
git checkout $BRANCH


echo @@@
echo @@@"Building linux 64 binaries"
echo @@@

mkdir -p ~/release
cd ~/soteria/depends
make HOST=x86_64-linux-gnu $MAKEOPTS
cd ~/soteria
export PATH=$PWD/depends/x86_64-linux-gnu/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-linux-gnu/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g" LDFLAGS="-static-libstdc++"
make $MAKEOPTS 
make -C src check-security
make -C src check-symbols 
mkdir ~/linux64
make install DESTDIR=~/linux64/$DISTNAME
cd ~/linux64
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find ${DISTNAME}/bin -type f -executable -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find ${DISTNAME}/lib -type f -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find $DISTNAME/ -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/release/$DISTNAME-x86_64-linux-gnu.tar.gz
cd ~/soteria
rm -rf ~/linux64
make clean
export PATH=$PATH_orig


echo @@@
echo @@@"Building general sourcecode"
echo @@@

cd ~/soteria
export PATH=$PWD/depends/x86_64-linux-gnu/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-linux-gnu/share/config.site ./configure --prefix=/
make dist
SOURCEDIST=`echo soteria-*.tar.gz`
mkdir -p ~/soteria/temp
cd ~/soteria/temp
tar xf ../$SOURCEDIST
find soteria-* | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ../$SOURCEDIST
cd ~/soteria
mv $SOURCEDIST ~/release
rm -rf temp
make clean
export PATH=$PATH_orig


echo @@@
echo @@@"Building linux 32 binaries"
echo @@@

cd ~/
mkdir -p ~/wrapped/extra_includes/i686-pc-linux-gnu
ln -s /usr/include/x86_64-linux-gnu/asm ~/wrapped/extra_includes/i686-pc-linux-gnu/asm

for prog in gcc g++; do
rm -f ~/wrapped/${prog}
cat << EOF > ~/wrapped/${prog}
#!/usr/bin/env bash
REAL="`which -a ${prog} | grep -v $PWD/wrapped/${prog} | head -1`"
for var in "\$@"
do
  if [ "\$var" = "-m32" ]; then
    export C_INCLUDE_PATH="$PWD/wrapped/extra_includes/i686-pc-linux-gnu"
    export CPLUS_INCLUDE_PATH="$PWD/wrapped/extra_includes/i686-pc-linux-gnu"
    break
  fi
done
\$REAL \$@
EOF
chmod +x ~/wrapped/${prog}
done

export PATH=$PWD/wrapped:$PATH
export HOST_ID_SALT="$PWD/wrapped/extra_includes/i386-linux-gnu"
cd ~/soteria/depends
make HOST=i686-pc-linux-gnu $MAKEOPTS
unset HOST_ID_SALT
cd ~/soteria
export PATH=$PWD/depends/i686-pc-linux-gnu/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/i686-pc-linux-gnu/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g" LDFLAGS="-static-libstdc++"
make $MAKEOPTS 
make -C src check-security
make -C src check-symbols 
mkdir -p ~/linux32
make install DESTDIR=~/linux32/$DISTNAME
cd ~/linux32
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find ${DISTNAME}/bin -type f -executable -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find ${DISTNAME}/lib -type f -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find $DISTNAME/ -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/release/$DISTNAME-i686-pc-linux-gnu.tar.gz
cd ~/soteria
rm -rf ~/linux32
rm -rf ~/wrapped
make clean
export PATH=$PATH_orig


echo @@@
echo @@@ "Building linux ARM binaries"
echo @@@

cd ~/soteria/depends
make HOST=arm-linux-gnueabihf $MAKEOPTS
cd ~/soteria
export PATH=$PWD/depends/arm-linux-gnueabihf/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/arm-linux-gnueabihf/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g" LDFLAGS="-static-libstdc++"
make $MAKEOPTS 
make -C src check-security
mkdir -p ~/linuxARM
make install DESTDIR=~/linuxARM/$DISTNAME
cd ~/linuxARM
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find ${DISTNAME}/bin -type f -executable -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find ${DISTNAME}/lib -type f -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find $DISTNAME/ -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/release/$DISTNAME-arm-linux-gnueabihf.tar.gz
cd ~/soteria
rm -rf ~/linuxARM
make clean
export PATH=$PATH_orig


echo @@@
echo @@@ "Building linux aarch64 binaries"
echo @@@

cd ~/soteria/depends
make HOST=aarch64-linux-gnu $MAKEOPTS
cd ~/soteria
export PATH=$PWD/depends/aarch64-linux-gnu/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/aarch64-linux-gnu/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g" LDFLAGS="-static-libstdc++"
make $MAKEOPTS 
make -C src check-security
mkdir -p ~/linuxaarch64
make install DESTDIR=~/linuxaarch64/$DISTNAME
cd ~/linuxaarch64
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find ${DISTNAME}/bin -type f -executable -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find ${DISTNAME}/lib -type f -exec ../soteria/contrib/devtools/split-debug.sh {} {} {}.dbg \;
find $DISTNAME/ -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/release/$DISTNAME-aarch64-linux-gnu.tar.gz
cd ~/soteria
rm -rf ~/linuxaarch64
make clean
export PATH=$PATH_orig


echo @@@
echo @@@ "Building windows 64 binaries"
echo @@@

update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix 
mkdir -p ~/release/unsigned/
mkdir -p ~/sign/win64
PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
cd ~/soteria/depends
make HOST=x86_64-w64-mingw32 $MAKEOPTS
cd ~/soteria
export PATH=$PWD/depends/x86_64-w64-mingw32/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g"
make $MAKEOPTS 
make -C src check-security
make deploy
rename 's/-setup\.exe$/-setup-unsigned.exe/' *-setup.exe
cp -f soteria-*setup*.exe ~/release/unsigned/
mkdir -p ~/win64
make install DESTDIR=~/win64/$DISTNAME
cd ~/win64
mv ~/win64/$DISTNAME/bin/*.dll ~/win64/$DISTNAME/lib/
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find $DISTNAME/bin -type f -executable -exec x86_64-w64-mingw32-objcopy --only-keep-debug {} {}.dbg \; -exec x86_64-w64-mingw32-strip -s {} \; -exec x86_64-w64-mingw32-objcopy --add-gnu-debuglink={}.dbg {} \;
find ./$DISTNAME -not -name "*.dbg"  -type f | sort | zip -X@ ./$DISTNAME-x86_64-w64-mingw32.zip
mv ./$DISTNAME-x86_64-*.zip ~/release/$DISTNAME-win64.zip
cd ~/
rm -rf win64
cp -rf soteria/contrib/windeploy ~/sign/win64
cd ~/sign/win64/windeploy
mkdir -p unsigned
mv ~/soteria/soteria-*setup-unsigned.exe unsigned/
find . | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/sign/$DISTNAME-win64-unsigned.tar.gz
cd ~/sign
rm -rf win64
cd ~/soteria
rm -rf release
make clean
export PATH=$PATH_orig


echo @@@
echo @@@ "Building windows 32 binaries"
echo @@@

update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix 
mkdir -p ~/sign/win32
PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') 
cd ~/soteria/depends
make HOST=i686-w64-mingw32 $MAKEOPTS
cd ~/soteria
export PATH=$PWD/depends/i686-w64-mingw32/native/bin:$PATH
./autogen.sh
CONFIG_SITE=$PWD/depends/i686-w64-mingw32/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-reduce-exports --disable-bench --disable-gui-tests CFLAGS="-O2 -g" CXXFLAGS="-O2 -g"
make $MAKEOPTS 
make -C src check-security
make deploy
rename 's/-setup\.exe$/-setup-unsigned.exe/' *-setup.exe
cp -f soteria-*setup*.exe ~/release/unsigned/
mkdir -p ~/win32
make install DESTDIR=~/win32/$DISTNAME
cd ~/win32
mv ~/win32/$DISTNAME/bin/*.dll ~/win32/$DISTNAME/lib/
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find $DISTNAME/bin -type f -executable -exec i686-w64-mingw32-objcopy --only-keep-debug {} {}.dbg \; -exec i686-w64-mingw32-strip -s {} \; -exec i686-w64-mingw32-objcopy --add-gnu-debuglink={}.dbg {} \;
find ./$DISTNAME -not -name "*.dbg"  -type f | sort | zip -X@ ./$DISTNAME-i686-w64-mingw32.zip
mv ./$DISTNAME-i686-w64-*.zip ~/release/$DISTNAME-win32.zip
cd ~/
rm -rf win32
cp -rf soteria/contrib/windeploy ~/sign/win32
cd ~/sign/win32/windeploy
mkdir -p unsigned
mv ~/soteria/soteria-*setup-unsigned.exe unsigned/
find . | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/sign/$DISTNAME-win32-unsigned.tar.gz
cd ~/sign
rm -rf win32
cd ~/soteria
rm -rf release
make clean
export PATH=$PATH_orig


echo @@@
echo @@@ "Building OSX binaries"
echo @@@

mkdir -p ~/soteria/depends/SDKs
cp ~/MacOSX11.3.sdk.tar.gz ~/soteria/depends/SDKs/MacOSX11.3.sdk.tar.gz
cd ~/soteria/depends/SDKs && tar -xf MacOSX11.3.sdk.tar.gz 
rm -rf MacOSX11.3.sdk.tar.gz 
cd ~/soteria/depends
make $MAKEOPTS HOST="x86_64-apple-darwin"
cd ~/soteria
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-apple-darwin/share/config.site ./configure --prefix=/ --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-reduce-exports --disable-bench --disable-gui-tests GENISOIMAGE=$PWD/depends/x86_64-apple-darwin/native/bin/genisoimage
make $MAKEOPTS 
mkdir -p ~/OSX
export PATH=$PWD/depends/x86_64-apple-darwin/native/bin:$PATH
make install-strip DESTDIR=~/OSX/$DISTNAME
make osx_volname
make deploydir
mkdir -p unsigned-app-$DISTNAME
cp osx_volname unsigned-app-$DISTNAME/
cp contrib/macdeploy/detached-sig-apply.sh unsigned-app-$DISTNAME
cp contrib/macdeploy/detached-sig-create.sh unsigned-app-$DISTNAME
cp $PWD/depends/x86_64-apple-darwin/native/bin/dmg $PWD/depends/x86_64-apple-darwin/native/bin/genisoimage unsigned-app-$DISTNAME
cp $PWD/depends/x86_64-apple-darwin/native/bin/x86_64-apple-darwin-codesign_allocate unsigned-app-$DISTNAME/codesign_allocate
cp $PWD/depends/x86_64-apple-darwin/native/bin/x86_64-apple-darwin-pagestuff unsigned-app-$DISTNAME/pagestuff
mv dist unsigned-app-$DISTNAME
cd unsigned-app-$DISTNAME
find . | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/sign/$DISTNAME-osx-unsigned.tar.gz
cd ~/soteria
make deploy
$PWD/depends/x86_64-apple-darwin/native/bin/dmg dmg "Soteria-Core.dmg" ~/release/unsigned/$DISTNAME-osx-unsigned.dmg
rm -rf unsigned-app-$DISTNAME dist osx_volname dpi36.background.tiff dpi72.background.tiff
cd ~/OSX
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf $DISTNAME/lib/pkgconfig
find $DISTNAME | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ~/release/$DISTNAME-osx64.tar.gz
cd ~/soteria
rm -rf ~/OSX
make clean
export PATH=$PATH_orig
