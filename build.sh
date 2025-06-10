#!/bin/sh

echo "Checking for old build"
DIRECTORY="build"
if [ -d "$DIRECTORY" ] && [ "$1" = "clean" ]; then
	echo "Deleting the old build"
	rm -rf build
	echo "Deleted the old build"
	mkdir build 
else
	echo "Skipping deletion of build folder..."
	if ! [ -d "$DIRECTORY" ]; then
	    mkdir build
	fi 
fi



cd build || exit
cmake ..
make || exit

printf "\n\n\033[0;32mSuccessfully Built the project. Running the Executable\033[0m\n\n"
if [ "$1" = "gdb" ]; then
	gdb hyprsic
else
	./hyprsic
fi