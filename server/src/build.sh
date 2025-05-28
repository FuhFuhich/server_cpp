#!/bin/bash
# build.sh

set -e

echo "Building Warehouse Server..."

mkdir -p build
cd build

cmake ..

make -j$(nproc)

echo "Build completed successfully!"
echo "Executable: $(pwd)/warehouse_server"
