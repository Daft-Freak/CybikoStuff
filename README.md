# Misc Cybiko Related Tools/Libraries

Possibly useful stuff pulled out of a huge pile of hacks. Most of this is a few years old.

## Tools
- `imgtool` - Converts executable images to the right format for booting
- `usbcon` - Uploads/boots files through the Cybike Xtreme USB port, can also access the USB console

### Build
```
cd tools
cmake -G Ninja -B build
ninja -C build
```

## Toolchain
GCC/newlib based toolchain build script, linker script, startup code and CMake config

To build the toolchain to `toolchain/prefix`: (`toolchain/build` can be deleted after this completes.)
```
cd toolchain
./build.sh
```

## Lib

### H8
C++ wrappers for accessing the CPU's peripherals.