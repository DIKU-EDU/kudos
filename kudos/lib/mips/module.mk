# Makefile for the lib module

# Set the module name
MODULE := lib/mips

FILES := rand.S

MIPSSRC += $(patsubst %, $(MODULE)/%, $(FILES))
