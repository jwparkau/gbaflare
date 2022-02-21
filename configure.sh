#!/bin/bash

mkdir -p build && cd build

if [ -z "$1" ]; then
	platform="SDL2"
elif [ "$1" = "SDL" ] || [ "$1" = "SDL2" ]; then
	platform="SDL2"
else
	echo "supported platforms"
	echo "SDL"
	echo "SDL2"
	exit 1
fi

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release -DUSE_"$platform"=1 -DSINGLE_SOURCE=0 ..
