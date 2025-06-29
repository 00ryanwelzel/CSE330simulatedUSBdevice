# CSE330simulatedUSBdevice

## (Ryan Welzel 6/29/2025)

OS class backup. Implements a block abstraction read/write interface to a USB device, using a custom IOCTL interface

---

Features:
- USB device handling


Requirements:
- Root acces to /dev/sdX (1GB virtual disk accessible at /dev/sdc for safety)
- Custom ioctl-defines.h for testing
- No AppArmor/SELinux restrictions
- Linux version 6.10 otherwise bio_XXX operations fail
- GCC compiler

Note: Within testing directory:
- Nest .c files within /.../kmodules
- Nest .h file within /.../
- Redefine path to .h file within kmod-ioctl.c
