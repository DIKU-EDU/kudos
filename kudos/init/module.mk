# Makefile for the kernel module

# Set the module name
MODULE := init

FILES := common.c

SRC += $(patsubst %, $(MODULE)/%, $(FILES))

