# Makefile for the kernel module

# Set the module name
MODULE := vm/mips

FILES := _tlb.S mm_virt.c tlb.c mm_phys.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

