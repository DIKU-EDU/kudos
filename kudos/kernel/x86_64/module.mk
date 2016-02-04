# Makefile for the drivers module

# Set the module name
MODULE := kernel/x86_64

FILES := _irq.S _spinlock.c cswitch.c interrupt.c stubs.c \
	 gdt.c idt.c exception.c pic.c tss.c spinlock.S

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))
