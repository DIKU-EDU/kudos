# Makefile for the kernel module

# Set the module name
MODULE := init/mips32


FILES := _boot.S main.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

