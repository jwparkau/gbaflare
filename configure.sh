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
#-DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release -DUSE_"$platform"=1 ..
