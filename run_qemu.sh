#! /bin/bash

ISO_PATH=./qemu/kudos.iso 
KUDOS_PATH=./kudos/kudos-x86_64

if [ -f $ISO_PATH ]; then
    rm $ISO_PATH
fi

cp -f $KUDOS_PATH ./qemu/grub/iso/boot/kudos-x86_64

grub-mkrescue -o $ISO_PATH ./qemu/grub/iso

qemu-system-x86_64 -monitor stdio -boot d -m 128 -cdrom  $ISO_PATH -net nic,vlan=0 -net user,vlan=0 -drive format=raw,file='./store.file',bus=1
