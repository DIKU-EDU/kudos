# Makefile for the kernel module

# Set the module name
MODULE := proc

FILES := elf.c syscall.c process.c

SRC += $(patsubst %, $(MODULE)/%, $(FILES))

