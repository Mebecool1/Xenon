#!/bin/bash

# --- 1. Root/Sudo Verification ---
if [ "$EUID" -ne 0 ]; then 
  echo "Error: Please run as root or using sudo."
  exit 1
fi

echo "Starting installation of xenc..."

# --- 2. Install the Binary ---
if [ -f "./xenc" ]; then
    echo "Installing binary: xenc -> /usr/local/bin/"
    cp ./xenc /usr/local/bin/
    chmod +x /usr/local/bin/xenc
else
    echo "Error: 'xenc' binary not found in current directory."
    exit 1
fi

# --- 3. Install the Standard Library ---
if [ -d "./stdlib" ]; then
    echo "Installing stdlib: stdlib/* -> /usr/include/xenon/"
    # Create the directory if it doesn't exist
    mkdir -p /usr/include/xenon/
    # Copy all files from stdlib into the destination
    cp -r ./stdlib/. /usr/include/xenon/
else
    echo "Error: 'stdlib/' directory not found."
    exit 1
fi

echo "Installation successful!"
echo "You can now run 'xenc' from any terminal."