#!/bin/bash
set -x

make

# load the files into the hardisk
dd if=$HOME/OS2016/AIM/boot/boot.bin of=$HOME/OS2016/100m.img bs=452 count=1 conv=notrunc
dd if=$HOME/OS2016/AIM/kern/vmaim.elf of=$HOME/OS2016/100m.img seek=20480 bs=512 count=1000 conv=notrunc

# open a new shell to use gdb
gnome-terminal -t "my_gdb" -x zsh -c "i386-unknown-elf-gdb $HOME/OS2016/AIM/kern/vmaim.elf -x gdb.cmd;"

# now run it!
qemu-system-i386 $HOME/OS2016/100m.img -serial stdio -smp 4 -gdb tcp::1234 -S

