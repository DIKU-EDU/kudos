# Makefile for the kernel module

# Set the module name
MODULE := init/mips


FILES := _boot.S main.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

