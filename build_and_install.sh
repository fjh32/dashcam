#!/bin/bash
set -e

RECORDINGS_DIR="/var/lib/dashcam/recordings/"
REAL_USER=$(logname)

echo "Creating recordings directory at $RECORDINGS_DIR..."
sudo mkdir -p "$RECORDINGS_DIR"
sudo chown -R "$USER:$USER" "$RECORDINGS_DIR"

# Clean build
rm -rf release_build

# Check for debug mode
if [[ "$1" == "--desktop" ]]; then
    echo "🛠️  Debug mode selected. Building without -rpi flag."
    ./build.sh -r
    BUILD_DIR="release_build"
else
    echo "🚀 Release mode with Raspberry Pi encoding enabled."
    ./build.sh -r -rpi
    BUILD_DIR="release_build"
fi

# Install systemd service and binary
echo "📝 Installing systemd service..."
sed "s|@USER@|$REAL_USER|g" dashcam.service.template | sudo tee /etc/systemd/system/dashcam.service > /dev/null

echo "📦 Installing binary to /usr/local/bin..."
cd "$BUILD_DIR"
sudo make install
cd ..

# Reload and start systemd service
echo "🔄 Reloading and enabling service..."
sudo systemctl daemon-reload
sudo systemctl enable dashcam.service
sudo systemctl start dashcam.service

echo
echo "✅ Installation complete."
echo "You can check the status of the service with:"
echo "  sudo systemctl status dashcam.service"
echo
