#!/bin/bash
# export_ascii.sh - Display the ASCII schematic over SSH
# Prefers img2txt or catimg if available, otherwise falls back to raw ASCII.

ASCII_FILE="ascii_schematic.txt"
PNG_FILE="schematic.png"

if command -v img2txt >/dev/null 2>&1; then
    echo "Using img2txt to convert PNG to ASCII:"
    img2txt "$PNG_FILE"
elif command -v catimg >/dev/null 2>&1; then
    echo "Using catimg to display PNG:"
    catimg "$PNG_FILE"
else
    echo "No image-to-ASCII tools found. Showing raw ASCII schematic:"
    cat "$ASCII_FILE"
fi