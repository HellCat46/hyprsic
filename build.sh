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
cd ..

printf "\n\n\033[0;32mSuccessfully Built the project. Running the Executable\033[0m\n\n"
if [ "$1" = "gdb" ]; then
	gdb build/hyprsic
elif [ "$1" = "valgrind" ]; then
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose build/hyprsic
else
	./build/hyprsic
fi