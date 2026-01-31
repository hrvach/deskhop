#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Get the directory of this script so we can use relative paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
WEBCONFIG_DIR="$PROJECT_ROOT/webconfig"
DISK_DIR="$PROJECT_ROOT/disk"

# --- 0. Clean previous virtual environments ---
echo "--- Cleaning up old virtual environments ---"
rm -rf "$WEBCONFIG_DIR/venv"
rm -rf "$DISK_DIR/venv"
echo "Cleanup complete."
echo ""

# --- 1. Render the web configuration page ---
echo "--- Running Webconfig Renderer ---"
pushd "$WEBCONFIG_DIR" > /dev/null

VENV_DIR="venv"
echo "Creating new virtual environment..."
python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"
pip install -r requirements.txt

python3 -B render.py
deactivate
echo "Render complete."
popd > /dev/null
echo ""

# --- 2. Generate the disk image ---
echo "--- Generating Disk Image ---"
pushd "$DISK_DIR" > /dev/null

VENV_DIR="venv"
echo "Creating new virtual environment..."
python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"
pip install -r requirements.txt

python3 create_disk.py
deactivate
echo "Disk image generation complete."
popd > /dev/null
