#!/bin/bash

# Expand aliases
shopt -s expand_aliases

# Give the possibility to provide an alternate cmake installation
if [ -v CMAKE ]; then
    echo "cmake is $CMAKE"
else
    echo "Set cmake to default"
    CMAKE="cmake"
fi

Help()
{
    # Display Help
    echo "Build script for librir"
    echo
    echo "Usage:"
    echo "--help			display help"
    echo "--with			download and compile the given plugin using its git url"
    echo "--debug		debug build only (default is release only)"
    echo "--build-wheel	build Python wheel package"
    echo "--global		global installation instead of local one"
    echo
}

function clone_pull {
    DIRECTORY=$(basename "$1" .git)
    if [ -d "$DIRECTORY" ]; then
        cd "$DIRECTORY"
        git pull
        cd ../
    else
        git clone "$1"
    fi
}


# Additional cmake options
CMAKE_OPTIONS="-DCMAKE_INSTALL_PREFIX=install"
CMAKE_BUILD="Release"
# Set to 1 to build wheel package
WHEEL=0

# Parse command line options
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            Help
            exit 0
            shift # past value
        ;;
        -w|--with)
            URL="$2"
            cd plugins
            clone_pull $URL
            cd ..
            shift # past argument
            shift # past value
        ;;
        -d|--debug)
            CMAKE_BUILD="Debug"
            shift # past value
        ;;
        --build-wheel)
            WHEEL=1
            shift # past value
        ;;
        --global)
            CMAKE_OPTIONS="-DLOCAL_INSTALL=OFF"
            shift # past value
        ;;
        -*|--*)
            echo "Unknown option $1"
            Help
            exit 1
        ;;
        *)
            Help
            exit 0
        ;;
    esac
done

CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_BUILD_TYPE=$CMAKE_BUILD"

# Build

mkdir -p build
cd build
$CMAKE ./.. -G "Unix Makefiles" $CMAKE_OPTIONS
make -j
make install

# Build wheel if necessary
if [ "$WHEEL" -eq "1" ]; then
    echo "Build wheel"
    cd install/python
    python -c "import librir;print('librir is importable !')"
    pip wheel --no-deps .
    cd ..
fi

cd ..
