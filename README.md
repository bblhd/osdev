pOSsum
===

This is my personal hobbyist operating system, its not very good yet, but I am trying to fix that.

It is inspired by the operating system plan9 (from bell labs), microkernels, the '[suckless](https://suckless.org/)' community,
and ideas such as [Permacomputing](https://permacomputing.net/). I hope to create a system that is accessible,
highly usable in simple ways, and maybe even at times, beautiful.

PS: the name specifically refers to the [Australian possum](https://en.wikipedia.org/wiki/Phalangeriformes), rather than the [American opossum](https://en.wikipedia.org/wiki/Opossum).

Building
---
The build script is written for linux and tested on bash. 

You can compile the kernel and "install" it using the build script like so.

```
./build.sh <target>
```

Replace `<target>` with either `iso`, `qemu`, or the device file name.
`iso` will only make the iso file, wheras `qemu` will also run it in qemu.
If you use a device file (for example `/dev/sdb`), it will use dd to write the iso to the specified device.

Requirements (for building)
---
* i686-elf-gcc
* nasm
* grub-mkrescue
* dd, sudo, other normal utilities

Thanks
---
* The fantastic [Repository](https://github.com/alpn/x86_starterkit)
by [alpn](https://github.com/alpn) which I used as a base for this project
* The incredible [OSDev.org](https://wiki.osdev.org/) wiki
