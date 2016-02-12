# Add your _userland_ program sources to the SOURCES variable.

SOURCES :=  halt.c shell.c hw.c

MIPS32INC = -I../kudos/lib/mips32/
X86_64INC = -I../kudos/lib/x86_64/

MIPS32OBJS := $(patsubst %.c, %.mips32.o, $(SOURCES))
X86_64OBJS  := $(patsubst %.c, %.x86_64.o, $(SOURCES))

MIPS32TARGETS := $(patsubst %.mips32.o, %.mips32, $(MIPS32OBJS))
X86_64TARGETS  := $(patsubst %.x86_64.o, %.x86_64, $(X86_64OBJS))

# crt.o must be the first one and the $(SYSLIBS) must come first in
# the pre-requisites list (or object files list).
MIPS32SYSLIBS := mips32/crt.mips32.o mips32/_syscall.mips32.o lib.mips32.o
X86_64SYSLIBS := x86_64/crt.x86_64.o x86_64/_syscall.x86_64.o lib.x86_64.o

# Compiler configuration
CC      := mips-elf-gcc
AS      := mips-elf-as
LD      := mips-elf-ld
CFLAGS  += -G0 -I../kudos -Wall -W -Werror -fno-builtin
LDFLAGS := --script=mips32/ld.userland.script --cref -s -G0
ASFLAGS := -I../kudos -Wa,-mips32

CCX86_64	:= x86_64-elf-gcc
ASX86_64	:= x86_64-elf-as
LDX86_64	:= x86_64-elf-ld

CFLAGSX86_64  += -O2 -I../kudos -Wall -Werror -Wno-attributes -m64 -mcmodel=small -mno-red-zone -finline-functions -fno-stack-protector -ffreestanding
LDFLAGSX86_64 := --script=x86_64/ld.userland.script -z max-page-size=0x1000
ASFLAGSX86_64 := -I../kudos

# -G0 is needed to avoid GP optimization (LD won't link if it is used)

# Compiling targets (Add explicit targets here if you need to do more
# complex linking (e.g., your source code is divided to multiple
# files))
all: mips32

mips32: $(MIPS32TARGETS)

x86_64: $(X86_64TARGETS)

# MIPS32 Targets
%.mips32: $(MIPS32SYSLIBS) %.mips32.o
	$(LD) -o $@ $(LDFLAGS) -Map $@.map $^

%.mips32.o: %.S
	$(CC) $(MIPS32INC) -o $@ $(ASFLAGS) -c $<

%.mips32.o: %.c
	$(CC) $(MIPS32INC) -o $@  $(CFLAGS) -c $<

# X86_64 Targets
%.x86_64: $(X86_64SYSLIBS) %.x86_64.o
	$(LDX86_64) -o $@ $(LDFLAGSX86_64) -Map $@.x64map $^

%.x86_64.o: %.S
	$(CCX86_64) $(X86_64INC) -o $@ $(ASFLAGSX86_64) -c $<

%.x86_64.o: %.c
	$(CCX86_64) $(X86_64INC) -o $@  $(CFLAGSX86_64) -c $<

clean:
	rm -f $(MIPS32SYSLIBS) $(MIPS32OBJS) *.map $(MIPS32TARGETS)
	rm -f $(X86_64SYSLIBS) $(X86_64OBJS) *.map $(X86_64TARGETS)

.PHONY: all mips32 x86_64 clean
