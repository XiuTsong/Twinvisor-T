#!/bin/bash

out_dir="../out"

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
# make all -j$(nproc)
make all

if [ $? -eq 0 ]; then
    echo "Make succeed\n"
else
    echo "Make failed\n"
fi
