#!/bin/sh

echo "Checking for old build"
DIRECTORY="build"
if [ -d "$DIRECTORY" ] && [ "$1" = "clean" ]; then
	echo "Deleting the old build"
	rm -rf build
	echo "Deleted the old build"
else
	echo "Skipping deletion of build folder..."
fi

# Checking for all the icons and adding them to the resources/icons.hpp

cd resources/icons || exit
for file in $(find ./ -type f -name "*.png"); do
    constName=$(sed -e "s/\//_/g" -e "s/\./_/g" <<< "${file:2:-4}")
    if grep -q $constName ../../src/resources/icons.hpp; then
        continue
    fi

    echo "Adding $file as $constName to icons.hpp"
    xxd -i -n "$constName" "$file"  | sed 's/unsigned char/const unsigned char/g; s/unsigned int/const unsigned int/g' >> ../../src/resources/icons.hpp
done
cd ../.. || exit

# Building All the Wayland Protocol Definitions
for file in $(find ./resources/wayland -name "*.xml"); do
    if [ ! -f "src/wayland/$(basename "${file%.xml}.h")" ] || [ "$file" -nt "src/wayland/$(basename "${file%.xml}.h")" ]; then
    echo "Processing $file"
    wayland-scanner client-header "$file" "src/wayland/$(basename "${file%.xml}.h")" || exit
    wayland-scanner private-code "$file" "src/wayland/$(basename "${file%.xml}.c")" || exit
    fi
done


if ! [ -d "$DIRECTORY" ]; then
    if [ "$1" = "prod" ]; then
        echo "Building in Production Mode"
        meson setup build --buildtype=release --wipe || exit
    else
        meson setup build --buildtype=debug --wipe || exit
    fi
fi

ninja -C build || exit

printf "\n\n\033[0;32mSuccessfully Built the project. Running the Executable\033[0m\n\n"
if [ "$1" = "gdb" ]; then
	gdb build/hyprsic
elif [ "$1" = "valgrind" ]; then
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose build/hyprsic
else
    if [ "$1" = "prod" ]; then
        echo "Running in Production Mode"
        DEBUG_MODE=0 build/hyprsic
    else
        echo "Running in Debug Mode"
        DEBUG_MODE=1 build/hyprsic
    fi
fi
