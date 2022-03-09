#!/bin/bash

mkdir -p build && cd build

if [ -z "$1" ]; then
	platform="QT5"
elif [ "$1" = "QT5" ]; then
	platform="QT5"
else
	echo "supported platforms"
	echo "QT5"
	exit 1
fi

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release -DUSE_"$platform"=1 -DSINGLE_SOURCE=0 ..
