 #!/usr/bin/env bash

 # Execute this file to install the soteria cli tools into your path on macOS

 CURRENT_LOC="$( cd "$(dirname "$0")" ; pwd -P )"
 LOCATION=${CURRENT_LOC%Soteria-Qt.app*}

 # Ensure that the directory to symlink to exists
 sudo mkdir -p /usr/local/bin

 # Create symlinks to the cli tools
 sudo ln -s ${LOCATION}/Soteria-Qt.app/Contents/MacOS/soteriad /usr/local/bin/soteriad
 sudo ln -s ${LOCATION}/Soteria-Qt.app/Contents/MacOS/soteria-cli /usr/local/bin/soteria-cli
