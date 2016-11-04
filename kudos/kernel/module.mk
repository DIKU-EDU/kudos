# Makefile for the kernel module

# Set the module name
MODULE := kernel

FILES := panic.c thread.c scheduler.c sleepq.c semaphore.c halt.c stalloc.c klock.c

SRC += $(patsubst %, $(MODULE)/%, $(FILES))
