#!/bin/bash

# installs test dependencies
file=v1.0.0.tar.gz
curl -L -O https://github.com/soteria-network/soteria/refs/tags/$file
echo "hash placeholder  $file" | sha256sum -c
python3 -m pip install $file --user
rm -rf $file
