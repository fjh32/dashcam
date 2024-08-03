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

mkdir -p $DIR
cd $DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make