# Makefile for the kernel module

# Set the module name
MODULE := kernel/mips


FILES := _cswitch.S cswitch.c _interrupt.S _spinlock.S \
	idle.S interrupt.c exception.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))

