if [[ "$1" == "--release" || "$1" == "-r" ]]; then
    echo "Release mode"
    BUILD_TYPE="Release"
    rm -rf build/
else
    echo "Not release mode"
    BUILD_TYPE="Debug"
fi

mkdir -p build/
cd build/
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make