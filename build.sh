#!/bin/bash
set -e

# Default values
BUILD_TYPE="Debug"
DIR="build"
RECORDINGS_DIR="./recordings"
RPI_FLAG=""

# Check for release mode
if [[ "$1" == "--release" || "$1" == "-r" ]]; then
    echo "🔧 Release mode"
    BUILD_TYPE="Release"
    DIR="release_build"
    RECORDINGS_DIR="/var/lib/dashcam/recordings"
    rm -rf "$DIR"
else
    echo "🔧 Debug mode"
fi

# Detect RPI mode
if [[ "$2" == "-rpi" ]]; then
    echo "🐤 Raspberry Pi mode"
    RPI_FLAG="-DRPI_MODE=ON"
elif [[ "$2" == "-rpi0" ]]; then
    echo "🐤 Raspberry Pi Zero mode"  
    RPI_FLAG="-DRPI_ZERO_MODE=ON"
elif [[ "$2" == "-exp" ]]; then
    echo "🐤 EXPERIMENTAL MODE"  
    RPI_FLAG="-DEXPERIMENTAL_MODE=ON"
else
    echo "🖥️  Desktop build"
    RPI_FLAG=""
fi

mkdir -p "$DIR"
mkdir -p "$RECORDINGS_DIR"
cd "$DIR"

# Run cmake with optional RPI mode
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" $RPI_FLAG ..
make
