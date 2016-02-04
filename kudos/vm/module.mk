# Makefile for the kernel module

# Set the module name
MODULE := vm

FILES := memory.c

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))
X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))

