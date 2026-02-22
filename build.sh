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

# Building All the Wayland Protocol Definitions
for file in $(find ./resources/wayland -name "*.xml"); do
    if [ ! -f "src/wayland/$(basename "${file%.xml}.h")" ] || [ "$file" -nt "src/wayland/$(basename "${file%.xml}.h")" ]; then
    echo "Processing $file"
    wayland-scanner client-header "$file" "src/wayland/$(basename "${file%.xml}.h")" || exit
    wayland-scanner private-code "$file" "src/wayland/$(basename "${file%.xml}.c")" || exit
    fi
done

# Add -DCMAKE_BUILD_TYPE=Debug Flag for Debug build (Increases build time)
cmake -G Ninja -S . -B build  -DCMAKE_BUILD_TYPE=Debug || exit
cmake --build build || exit

printf "\n\n\033[0;32mSuccessfully Built the project. Running the Executable\033[0m\n\n"
if [ "$1" = "gdb" ]; then
	gdb build/hyprsic
elif [ "$1" = "valgrind" ]; then
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose build/hyprsic
else
	./build/hyprsic
fi
