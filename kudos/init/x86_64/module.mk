# Makefile for the kernel module

# Set the module name
MODULE := init/x86_64


FILES := _boot.S main.c 

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))

