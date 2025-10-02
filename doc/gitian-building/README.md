Gitian building
================

Gitian is the deterministic build process that is used to build the Soteria
executables. It provides a way to be reasonably sure that the
executables are really built from the git source. It also makes sure that
the same, tested dependencies are used and statically built into the executable.

Multiple developers build the source code by following a specific descriptor
("recipe"), cryptographically sign the result, and upload the resulting signature.
These results are compared and only if they match, the build is accepted and provided
for download.

More independent Gitian builders are needed, which is why this guide exists.

# Gitian building using Docker (Current)
Soteria gitian builds utilize docker images and containers.
More information about that process can be found here:
https://github.com/Soteria-Network/docker-based-gitian-builder


# Gitian building using a VM (Legacy)
*Maintained for legacy purposes. The docker process above should be used for all new builds.*
- *Setup instructions for a Gitian build of Soteria using a VM or physical system.*
- *You Should use Debian Bookworm for the VM host until further notice.*

Table of Contents
------------------
- [Preparing the Gitian builder host](#preparing-the-gitian-builder-host)
- [Initial Gitian Setup](#initial-gitian-setup)
- [Building Soteria Binaries](#building-soteria-binaries)
- [Signing Externally](#signing-externally)
- [Uploading Signatures](#uploading-signatures)

Preparing the Gitian builder host
---------------------------------
The first step is to prepare the host environment that will be used to perform the Gitian builds.
This guide explains how to set up the environment, and how to start the builds.

Gitian builds are known to be working on recent versions of Debian, Ubuntu and Fedora.
If your machine is already running one of those operating systems, you can perform Gitian builds on the actual hardware.
Alternatively, you can install one of the supported operating systems in a virtual machine.

Any kind of virtualization can be used, for example:
- [VirtualBox](https://www.virtualbox.org/) (covered by this guide)
- [KVM](http://www.linux-kvm.org/page/Main_Page)
- [LXC](https://linuxcontainers.org/)

Please refer to the following documents to set up the operating systems and Gitian.

|                                   | Debian                                                                             | Fedora                                                                             |
|-----------------------------------|------------------------------------------------------------------------------------|------------------------------------------------------------------------------------|
| Setup virtual machine (optional)  | [Create Debian VirtualBox](./gitian-building-create-vm-debian.md) | [Create Fedora VirtualBox](./gitian-building-create-vm-fedora.md) |
| Setup Gitian                      | [Setup Gitian on Debian](./gitian-building-setup-gitian-debian.md) | [Setup Gitian on Fedora](./gitian-building-setup-gitian-fedora.md) |

Note that a version of `lxc-execute` higher or equal to 2.1.1 is required.
You can check the version with `lxc-execute --version`.
On Debian you might have to compile a suitable version of lxc or you can use Bookworm 12 or higher instead of Debian as the host.

Non-Debian / Ubuntu, Manual and Offline Building
------------------------------------------------
The instructions below use the automated script [gitian-build.py](https://github.com/Soteria-Network/soteria/blob/master/contrib/gitian-build.py) which only works in Debian/Ubuntu. For manual steps and instructions for fully offline signing, see [this guide](./gitian-building-manual.md).

MacOS code signing
------------------
In order to sign builds for MacOS, you need to download the free SDK and extract a file. The steps are described [here](./gitian-building-mac-os-sdk.md). Alternatively, you can skip the OSX build by adding `--os=lw` below. Or you can download a hosted version from here https://dl.dropboxusercontent.com/s/guon6ye5ciktqsk/MacOSX10.11.sdk.tar.gz

Initial Gitian Setup
--------------------
The `gitian-build.py` script will checkout different release tags, so it's best to copy it:

```bash
cp soteria/contrib/gitian-build.py .
```

You only need to do this once:

```
./gitian-build.py --setup satoshi 0.0.9.9
```

Where `satoshi` is your Github name and `0.0.9.9` is the most recent tag (without `v`). 

In order to sign gitian builds on your host machine, which has your PGP key, fork the gitian.sigs repository and clone it on your host machine:

```
git clone git@github.com:Soteria-Network/gitian.sigs.git
git remote add satoshi git@github.com:satoshi/gitian.sigs.git
```

Building Soteria Binaries
-----------------------------
Windows and OSX have code signed binaries, but those won't be available until a few developers have gitian signed the non-codesigned binaries.

To build the most recent tag:

 `./gitian-build.py --detach-sign --no-commit -b satoshi 0.0.9.9`

To speed up the build, use `-j 8 -m 6000` as the first arguments, where `8` is the number of CPU's you allocated to the VM plus one, and 6000 is a little bit less than then the MB's of RAM you allocated.


Signing Externally
-----------------------------
If all went well, this produces a number of (uncommited) `.assert` files in the gitian.sigs repository.

You need to copy these uncommited changes to your host machine, where you can sign them:

```
export NAME=satoshi
gpg --output $VERSION-linux/$NAME/soteria-linux-0.16-build.assert.sig --detach-sign 0.16.0rc1-linux/$NAME/soteria-linux-0.16-build.assert 
gpg --output $VERSION-osx-unsigned/$NAME/soteria-osx-0.16-build.assert.sig --detach-sign 0.16.0rc1-osx-unsigned/$NAME/soteria-osx-0.16-build.assert 
gpg --output $VERSION-win-unsigned/$NAME/soteria-win-0.16-build.assert.sig --detach-sign 0.16.0rc1-win-unsigned/$NAME/soteria-win-0.16-build.assert 
```

Make a PR (both the `.assert` and `.assert.sig` files) to the
[Soteria-Network/gitian.sigs](https://github.com/Soteria-Network/gitian.sigs/) repository:

```
git checkout -b 0.0.9.9-not-codesigned
git commit -S -a -m "Add $NAME 0.0.9.9 non-code signed signatures"
git push --set-upstream $NAME 0.0.9.9
```

Uploading Signatures
-----------------------------
You can also mail the files to Presstab (presstab1337@gmail.com) and he will commit them.

```bash
    gpg --detach-sign ${VERSION}-linux/${SIGNER}/soteria-linux-*-build.assert
    gpg --detach-sign ${VERSION}-win-unsigned/${SIGNER}/soteria-win-*-build.assert
    gpg --detach-sign ${VERSION}-osx-unsigned/${SIGNER}/soteria-osx-*-build.assert
```

You may have other .assert files as well (e.g. `signed` ones), in which case you should sign them too. You can see all of them by doing `ls ${VERSION}-*/${SIGNER}`.

This will create the `.sig` files that can be committed together with the `.assert` files to assert your
Gitian build.


 `./gitian-build.py --detach-sign -s satoshi 0.0.9.9 --nocommit`

Make another pull request for these.
