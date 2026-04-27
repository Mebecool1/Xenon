#!/bin/bash

# --- 1. Root/Sudo Verification ---
if [ "$EUID" -ne 0 ]; then 
  echo "Error: Please run as root or using sudo."
  exit 1
fi

echo "Starting removal of lcc..."

# --- 2. Remove the Binary ---
if [ -f "/usr/local/bin/lcc" ]; then
    echo "Removing binary: /usr/local/bin/lcc"
    rm /usr/local/bin/lcc
else
    echo "Notice: /usr/local/bin/lcc not found. Skipping."
fi

# --- 3. Remove the Standard Library ---
if [ -d "/usr/include/lb" ]; then
    echo "Removing stdlib directory: /usr/include/lb/"
    rm -rf /usr/include/lb/
else
    echo "Notice: /usr/include/lb/ not found. Skipping."
fi

echo "Uninstallation complete."