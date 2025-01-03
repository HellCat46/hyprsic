#!/bin/bash

echo "Checking for old build"
DIRECTORY="build"
if [ -d "$DIRECTORY" ]; then
	echo "Deleting the old build"
	rm -rf build
	echo "Deleted the old build"
else
	echo "No old build found"
fi

mkdir build && cd build
cmake ..
make

if [ "$1" == "gdb" ]; then
	gdb hyprsic
else
	./hyprsic
fi