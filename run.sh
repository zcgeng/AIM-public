#!/bin/bash
set -x

make clean
make
dd if=$HOME/OS2016/AIM/boot/boot.bin of=$HOME/OS2016/100m.img bs=446 count=1 conv=notrunc
dd if=$HOME/OS2016/AIM/kern/vmaim.elf of=$HOME/OS2016/100m.img seek=20480 bs=512 count=100 conv=notrunc
qemu-system-i386 $HOME/OS2016/100m.img -gdb tcp::1234 -S
