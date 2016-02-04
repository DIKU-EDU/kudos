# Makefile for the drivers module

# Set the module name
MODULE := vm/x86_64

FILES := mm_phys.c mm_virt.c

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))
