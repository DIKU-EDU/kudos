#!/usr/bin/env bash

set -euo pipefail

ISO_PATH=./qemu/kudos.iso
KUDOS_DISK_PATH=./store.file
KUDOS_PATH=./kudos/kudos-x86_64

if [ -f "$ISO_PATH" ]; then
  rm "$ISO_PATH"
fi

cp -f "$KUDOS_PATH" "./qemu/grub/iso/boot/kudos-x86_64"

grub-mkrescue -o "$ISO_PATH" "./qemu/grub/iso"

qemu-system-x86_64 \
  -gdb tcp::1234    \
  -monitor stdio    `# non graphical mode` \
  -m 128            `# megs of RAM` \
  -net nic          `# emulate a network interface card` \
  -netdev user      `# enable user mode networking` \
  -drive file="$ISO_PATH",if=ide,bus=0,unit=0,media=cdrom \
  -drive file="$KUDOS_DISK_PATH",format=raw,if=ide,bus=0,unit=1
