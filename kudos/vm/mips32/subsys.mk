# Makefile for the kernel module

# Set the module name
MODULE := vm/mips32

FILES := _tlb.S mm_virt.c tlb.c mm_phys.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

