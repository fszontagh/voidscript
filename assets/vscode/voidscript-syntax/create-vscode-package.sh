#!/bin/bash

# Check if 'vsce' (Visual Studio Code Extension Manager) is installed
if ! command -v vsce &> /dev/null
then
    echo "'vsce' not found! Please install it to proceed."
    echo "Install with: npm install -g vsce"
    exit 1
fi

# Check if the package.json file exists
if [ ! -f "package.json" ]; then
    echo "package.json not found. Are you running this script in the extension's root directory?"
    exit 1
fi

# Start packaging the extension
echo "Packaging extension..."

vsce package

# If packaging succeeded, inform the user
if [ $? -eq 0 ]; then
    echo "Extension package created successfully!"
else
    echo "Error: Packaging failed."
    exit 1
fi
