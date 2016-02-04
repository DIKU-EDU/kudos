# Makefile for the lib module

# Set the module name
MODULE := lib

FILES := libc.c xprintf.c bitmap.c debug.c

SRC += $(patsubst %, $(MODULE)/%, $(FILES))
