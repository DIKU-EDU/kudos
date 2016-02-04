# Makefile for the kernel module

# Set the module name
MODULE := proc/x86_64

FILES := _syscall.c _sys.S _proc.c

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))

