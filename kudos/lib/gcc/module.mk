# Makefile for the drivers module

# Set the module name
MODULE := lib/gcc

FILES := divdi3.c qdivrem.c udivdi3.c umoddi3.c

SRC += $(patsubst %, $(MODULE)/%, $(FILES))
