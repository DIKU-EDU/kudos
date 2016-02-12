# Add your _userland_ program sources to the SOURCES variable.

SOURCES :=  halt.c shell.c hw.c

MIPSINC = -I../kudos/lib/mips/
X64INC = -I../kudos/lib/x86_64/

MIPSOBJECTS := $(patsubst %.c, %.mips_o, $(SOURCES))
X64OBJECTS  := $(patsubst %.c, %.x86_o, $(SOURCES))

MIPSTARGETS := $(patsubst %.mips_o, %.mips, $(MIPSOBJECTS))
X64TARGETS  := $(patsubst %.x86_o, %.x86_64, $(X64OBJECTS))

# crt.o must be the first one and the $(SYSLIBS) must come first in
# the pre-requisites list (or object files list).
MIPSSYSLIBS := mips/crt.mips_o mips/_syscall.mips_o lib.mips_o
X64SYSLIBS := x86_64/crt.x86_o x86_64/_syscall.x86_o lib.x86_o

# Compiler configuration
CC      := mips-elf-gcc
AS      := mips-elf-as
LD      := mips-elf-ld
CFLAGS  += -G0 -I../kudos -Wall -W -Werror -fno-builtin
LDFLAGS := --script=mips/ld.userland.script --cref -s -G0
ASFLAGS := -I../kudos -Wa,-mips32

CC64	:= x86_64-elf-gcc
AS64	:= x86_64-elf-as
LD64	:= x86_64-elf-ld

CFLAGS64  += -O2 -I../kudos -Wall -Werror -Wno-attributes -m64 -mcmodel=small -mno-red-zone -finline-functions -fno-stack-protector -ffreestanding
LDFLAGS64 := --script=x86_64/ld.userland.script -z max-page-size=0x1000
ASFLAGS64 := -I../kudos

# -G0 is needed to avoid GP optimization (LD won't link if it is used)

# Compiling targets (Add explicit targets here if you need to do more
# complex linking (e.g., your source code is divided to multiple
# files))
all: $(MIPSTARGETS) $(X64TARGETS)

# Mips
%.mips: $(MIPSSYSLIBS) %.mips_o
	$(LD) -o $@ $(LDFLAGS) -Map $@.map $^

%.mips_o: %.S
	$(CC) $(MIPSINC) -o $@ $(ASFLAGS) -c $<

%.mips_o: %.c
	$(CC) $(MIPSINC) -o $@  $(CFLAGS) -c $<

# X64 Targets
%.x86_64: $(X64SYSLIBS) %.x86_o
	$(LD64) -o $@ $(LDFLAGS64) -Map $@.x64map $^

%.x86_o: %.S
	$(CC64) $(X64INC) -o $@ $(ASFLAGS64) -c $<

%.x86_o: %.c
	$(CC64) $(X64INC) -o $@  $(CFLAGS64) -c $<

clean:
	rm -f $(MIPSSYSLIBS) $(MIPSOBJECTS) *.map $(MIPSTARGETS) *~
	rm -f $(X64SYSLIBS) $(X64OBJECTS) *.map $(X64TARGETS) *~

.PHONY: clean
