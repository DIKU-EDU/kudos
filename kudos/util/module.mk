# Makefile for the util subdirectory

# Set the module name
MODULE := util

EXTRAINC      := -I./drivers/mips -I./drivers/x86_64 -I./vm/mips -I./vm/x86_64 -I./kernel/mips -I./kernel/x86_64
NATIVECC      := gcc
NATIVECFLAGS  += -O2 -g -I. -Wall -W
TARGETS       += util/tfstool

util/tfstool: util/tfstool.o
	$(NATIVECC) -o $@ $^

util/tfstool.o: util/tfstool.c util/tfstool.h fs/tfs.h lib/bitmap.h
	$(NATIVECC) $(EXTRAINC) -o $@  $(NATIVECFLAGS) -c $<

utilclean:
	rm -f util/*.[od] util/tfstool
