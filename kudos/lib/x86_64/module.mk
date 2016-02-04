# Makefile for the drivers module

# Set the module name
MODULE := lib/x86_64

FILES := asm.c srand.c _asm.S

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))
