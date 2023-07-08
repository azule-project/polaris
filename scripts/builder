#!/usr/bin/env bash

pushd external/
if [ ! -e boost-cmake ]; then
git clone https://github.com/Loser911/boost-cmake
fi
if [ ! -e source-sdk-2013-headers ]; then
git clone https://github.com/nullworks/source-sdk-2013-headers
fi
if [ ! -e libxoverlay ]; then
git clone https://github.com/nullworks/libxoverlay
fi
if [ ! -e TF2_NavFile_Reader ]; then
git clone https://github.com/nullworks/TF2_NavFile_Reader
fi
if [ ! -e clip ]; then
git clone https://github.com/nullworks/clip
fi
if [ ! -e MicroPather ]; then
git clone https://github.com/nullworks/MicroPather
fi
if [ ! -e simple-ipc ]; then
git clone https://github.com/Loser911/simple-ipc
fi
popd

mkdir -p ./obj
# Update cathook
pushd obj && cmake .. && cmake  --build . --target cathook -- -j"$(nproc --all)" || {
echo -e "\033[1;31m \n \nFailed to compile cathook\n\033[0m"
exit 1
}
# Update data
cwd="$(pwd)"
cmake --build . --target data || {
echo -e "\033[1;31m\nFailed to update /opt/cathook/data directory! Trying with root rights!\n\033[0m"
sudo bash -c "cd \"$cwd\"; cmake --build . --target data" || {
    echo -e "\033[1;31m\nFailed to update /opt/cathook/data directory\n\033[0m"
    exit 1
    }
}
popd
echo -e "\n\033[1;34mCathook has installed.\n\033[0m"
