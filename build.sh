#!/usr/bin/env bash

cd "$(dirname "$(readlink -f "$0")")" || exit 1

# TCP proxy
echo "Building toxiproxy"
cd ./toxiproxy || exit 1
make build
cd ..

# UDP proxy
echo -e "\nBuilding UDP proxy"
cd ./udp-proxy || exit 1
go build
cd ..

if ! [ -f ./build/Makefile ]; then
    echo -e "\nPreparing build environment for frontend + syncinput"
    mkdir -p build
    cd build || exit 1
    cmake -DCMAKE_BUILD_TYPE=Release ../src
    cd ..
fi

echo -e "\nBuilding frontend + syncinput"
cd ./build || exit 1
make -j
cd ..
