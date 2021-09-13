# Misc Cybiko Related Tools/Libraries

Possibly useful stuff pulled out of a huge pile of hacks.

## Tools
- `imgtool` - Converts executable images to the right format for booting
- `usbcon` - Uploads/boots files through the Cybike Xtreme USB port, can also access the USB console

### Build
```
cd tools
cmake -G Ninja -B build
ninja -C build
```