osdev
===

This is my personal hobbyist operating system, its not very good yet, but I am trying to fix that. 

It might be a bit silly and idiosyncratic sometimes. 

Also my custom build script may not be very robust, so yeah
(as in it may not work on your machine).
You may wonder why I didn't just use make.

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

Requirements
---
* i686-elf-gcc
* grub-mkrescue
* dd, sudo, other normal utilities

Todo
---
* [ ] improve all my lazily written important kernel code

Thanks
---
* The fantastic [Repository](https://github.com/alpn/x86_starterkit) by alpn
* The incredible [OSDev.org](https://wiki.osdev.org/) wiki
