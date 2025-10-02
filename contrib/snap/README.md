# Soteria Snap Packaging

Commands for building and uploading a Soteria Core Snap to the Snap Store. Anyone on amd64 (x86_64), arm64 (aarch64), or i386 (i686) should be able to build it themselves with these instructions. This would pull the official Soteria binaries from the releases page, verify them, and install them on a user's machine.

## Building Locally
```
sudo apt install snapd
sudo snap install --classic snapcraft
sudo snapcraft
```

### Installing Locally
```
snap install \*.snap --devmode
```

### To Upload to the Snap Store
```
snapcraft login
snapcraft register soteria-core
snapcraft upload \*.snap
sudo snap install soteria-core
```

### Usage
```
soteria-unofficial.cli # for soteria-cli
soteria-unofficial.d # for soteriad
soteria-unofficial.qt # for soteria-qt
soteria-unofficial.test # for test_soteria
soteria-unofficial.tx # for soteria-tx
```

### Uninstalling
```
sudo snap remove soteria-unofficial
```
