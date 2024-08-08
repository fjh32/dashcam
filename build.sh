if [[ "$1" == "--release" || "$1" == "-r" ]]; then
    echo "Release mode"
    BUILD_TYPE="Release"
    DIR=release_build/
    rm -rf $DIR
else
    echo "Not release mode"
    DIR=build/
    BUILD_TYPE="Debug"
fi

if [[ "$2" == "-rpi" ]]; then
    echo "Raspberry Pi mode"
    RPI_MODE="ON"
else
    echo "Not Raspberry Pi mode"
    RPI_MODE="OFF"
fi

mkdir -p $DIR
cd $DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DRPI_MODE=$RPI_MODE ..
make