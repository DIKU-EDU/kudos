# Makefile for the lib module

# Set the module name
MODULE := lib/mips32

FILES := rand.S

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))
