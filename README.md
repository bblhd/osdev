osdev
===

What
---
This is my personal hobbyist operating system, its not very good yet, but I am trying to fix that. 

It might be a bit silly and idiosyncratic sometimes. 

Also my custom build script mightn't be very robust, so yeah
(as in it might not work (but it works on my machine)).
You may wonder why I didn't just use make.

Building
---
Build the kernel and "install" it using the build.sh script like so.

`
./build.sh <target format>
`

Replace `<target format>` with either `iso`, `qemu`, or the device file name.
`iso` will only make the iso file, wheras `qemu` will also run it in qemu.
If you use a device file, it will use dd to write the iso to the specified device.

It is recomended to delete the build file every so often, especially after changing header files,
as the build script currently doesn't account for changed header files.
The build script also doesn't delete anything itself.

Requirements
---
* i686-elf-gcc
* grub-mkrescue
* dd, other normal utilities

Thanks
---
* The fantastic [Repository](https://github.com/alpn/x86_starterkit) by alpn
* The incredible [OSDev.org](https://wiki.osdev.org/) wiki
