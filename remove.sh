#!/bin/bash

# --- 1. Root/Sudo Verification ---
if [ "$EUID" -ne 0 ]; then 
  echo "Error: Please run as root or using sudo."
  exit 1
fi

echo "Starting removal of xenc..."

# --- 2. Remove the Binary ---
if [ -f "/usr/local/bin/xenc" ]; then
    echo "Removing binary: /usr/local/bin/xenc"
    rm /usr/local/bin/xenc
else
    echo "Notice: /usr/local/bin/xenc not found. Skipping."
fi

# --- 3. Remove the Standard Library ---
if [ -d "/usr/include/xenon" ]; then
    echo "Removing stdlib directory: /usr/include/xenon/"
    rm -rf /usr/include/xenon/
else
    echo "Notice: /usr/include/xenon/ not found. Skipping."
fi

echo "Uninstallation complete."