#!/bin/bash

# --- 1. Root/Sudo Verification ---
if [ "$EUID" -ne 0 ]; then 
  echo "Error: Please run as root or using sudo."
  exit 1
fi

echo "Starting installation of lcc..."

# --- 2. Install the Binary ---
if [ -f "./lcc" ]; then
    echo "Installing binary: lcc -> /usr/local/bin/"
    cp ./lcc /usr/local/bin/
    chmod +x /usr/local/bin/lcc
else
    echo "Error: 'lcc' binary not found in current directory."
    exit 1
fi

# --- 3. Install the Standard Library ---
if [ -d "./stdlib" ]; then
    echo "Installing stdlib: stdlib/* -> /usr/include/lb/"
    # Create the directory if it doesn't exist
    mkdir -p /usr/include/lb/
    # Copy all files from stdlib into the destination
    cp -r ./stdlib/. /usr/include/lb/
else
    echo "Error: 'stdlib/' directory not found."
    exit 1
fi

echo "Installation successful!"
echo "You can now run 'lcc' from any terminal."