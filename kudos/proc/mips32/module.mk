# Makefile for the kernel module

# Set the module name
MODULE := proc/mips32

FILES := exception.c _syscall.c _proc.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

