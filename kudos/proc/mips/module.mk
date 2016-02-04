# Makefile for the kernel module

# Set the module name
MODULE := proc/mips

FILES := exception.c _syscall.c _proc.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

