if [[ "$1" == "--release" || "$1" == "-r" ]]; then
    echo "Release mode"
    BUILD_TYPE="Release"
    DIR=release_build/
    RECORDINGS_DIR=/home/frank/recordings/
    mkdir -p ~/dashcam_web_static
    cp -r dashcam_web_static/* ~/dashcam_web_static
    rm -rf $DIR
else
    echo "Not release mode"
    DIR=build/
    BUILD_TYPE="Debug"
    RECORDINGS_DIR=./recordings/
fi

if [[ "$2" == "-rpi" ]]; then
    echo "Raspberry Pi mode"
    RPI_MODE="ON"
elif [[ "$2" == "-rpi0" ]]; then
    echo "Raspberry Pi Zero mode with Hardware Acceleration"
    RPI_MODE="ZERO"
else
    echo "Not Raspberry Pi mode"
    RPI_MODE="OFF"
fi

mkdir -p $DIR
mkdir -p $RECORDINGS_DIR
rm $RECORDINGS_DIR/*.ts
rm $RECORDINGS_DIR/*.m3u8
cd $DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DRPI_MODE=$RPI_MODE ..
make
