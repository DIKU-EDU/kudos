# Makefile for the drivers module

# Set the module name
MODULE := drivers/x86_64

FILES := polltty.c tty.c device.c _timer.S pit.c \
	keyboard.c _kb.S disk.c _disk.S metadev.c \
	pci.c

X64SRC += $(patsubst %, $(MODULE)/%, $(FILES))
