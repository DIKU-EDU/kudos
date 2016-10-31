# This Makefile skeleton is excerpted from the paper
# _Recursive Make Considered Harmful_ by Peter Miller
# <URL:http://www.tip.net.au/~millerp/rmch/recu-make-cons-harm.html>

## User configurable variables
MODULES    := init kernel lib drivers proc vm fs util lib/gcc
ARCHMIPS32 := init/mips32 kernel/mips32 vm/mips32 lib/mips32 drivers/mips32 proc/mips32
ARCHX86_64 := init/x86_64 kernel/x86_64 vm/x86_64 lib/x86_64 drivers/x86_64 proc/x86_64
#CHANGEDFLAGS := -DCHANGED_1 -DCHANGED_2 -DCHANGED_3 -DCHANGED_4 -DCHANGED_5

#IncludePaths
MIPS32_INC = -I./drivers/mips32/ -I./lib/mips32/ -I./kernel/mips32/ -I./vm/mips32/ -I./proc/mips32/
X86_64_INC = -I./drivers/x86_64/ -I./lib/x86_64/ -I./kernel/x86_64/ -I./vm/x86_64/ -I./proc/x86_64/

## Below this point, you shouldn't have to change anything.
TARGET_MIPS32 := kudos-mips32
TARGET_X86_64	:= kudos-x86_64
UTILTARGET 		:= util/tfstool

# Compiler and tar configuration
CC      := mips-elf-gcc
AS      := mips-elf-as
LD      := mips-elf-ld

SHARED_CFLAGS=-g -I. -Wall -Wextra -pedantic

CFLAGS  += $(SHARED_CFLAGS) -G0 $(CHANGEDFLAGS)
LDFLAGS := --script=ldmips32.script --cref -G0 -Map kudos-mips32.map
ASFLAGS := -gstabs+ -I. -Wa,-mips32 $(CHANGEDFLAGS)

CCX86_64	:= gcc
ASX86_64	:= as
LDX86_64	:= ld

CFLAGSX86_64  += $(SHARED_CFLAGS) -fverbose-asm -Wno-attributes -std=gnu99 -m64 -mcmodel=large -mno-red-zone -finline-functions -fno-stack-protector -ffreestanding -D SMALL_ENDIAN $(CHANGEDFLAGS)
LDFLAGSX86_64 := --script=ldx86_64.script -z max-page-size=0x1000 -Map kudos-x86_64.map
ASFLAGSX86_64 := -I.

# -G0 is needed to avoid GP optimization (LD won't link if it is used)
GTAR    := tar

# Each module adds to this
SRC     :=
MIPSSRC :=
X64SRC  :=

all: x86_64

mips32: $(TARGET_MIPS32) $(UTILTARGET)

x86_64: $(TARGET_X86_64) $(UTILTARGET)

install: $(TARGET64)
	mount /dev/sda6
	cp -f ./$(TARGET64) $(HOME)/kos/boot/kUdOS
	umount $(HOME)/kos

copy:
	dd if=$(IMG) of=/dev/sda7

# include the description for each module
include $(patsubst %, %/module.mk, $(MODULES))
include $(patsubst %, %/module.mk, $(ARCHMIPS32))
include $(patsubst %, %/module.mk, $(ARCHX86_64))

# determine the object files
MIPS32OBJS :=  $(patsubst %.s,%.mips32.o,$(filter %.s,$(SRC))) \
	 $(patsubst %.S,%.mips32.o,$(filter %.S,$(SRC))) \
         $(patsubst %.c,%.mips32.o,$(filter %.c,$(SRC))) \
         $(patsubst %.s,%.mips32.o,$(filter %.s,$(MIPSSRC))) \
	 $(patsubst %.S,%.mips32.o,$(filter %.S,$(MIPSSRC))) \
         $(patsubst %.c,%.mips32.o,$(filter %.c,$(MIPSSRC)))

X86_64OBJS := $(patsubst %.s,%.x86_64.o,$(filter %.s,$(X64SRC))) \
	$(patsubst %.S,%.x86_64.o,$(filter %.S,$(X64SRC))) \
        $(patsubst %.c,%.x86_64.o,$(filter %.c,$(X64SRC))) \
	$(patsubst %.s,%.x86_64.o,$(filter %.s,$(SRC))) \
	$(patsubst %.S,%.x86_64.o,$(filter %.S,$(SRC))) \
	$(patsubst %.c,%.x86_64.o,$(filter %.c,$(SRC)))

# link the program

# For some reason the order of the object file matters; the startup
# code (kernel/boot.o) must come before other files.
$(TARGET_MIPS32): $(MIPS32OBJS) 
	$(LD) -o $@ $(LDFLAGS) $^

# Compile rule for assembler source
%.mips32.o: %.s
	$(AS) -o $@ -mips32 $<

%.mips32.o: %.S
	$(CC) -o $@ $(ASFLAGS) -c $<

# Compile rule for C source
%.mips32.o: %.c
	$(CC) $(MIPS32_INC) -o $@  $(CFLAGS) -c $<

$(TARGET_X86_64): $(X86_64OBJS)
	$(LDX86_64) -o $@ $(LDFLAGSX86_64) $^

# Compile rule for assembler source
%.x86_64.o: %.s
	$(ASX86_64) -o $@ $<

%.x86_64.o: %.S
	$(CCX86_64) -o $@ $(ASFLAGSX86_64) -c $<

# Compile rule for C source
%.x86_64.o: %.c
	$(CCX86_64) $(X86_64_INC) -o $@  $(CFLAGSX86_64) -c $<


## Dependencies

# determine the dependencies files
DEPS := $(patsubst %.c,%.d,$(filter %.c,$(SRC))) \
        $(patsubst %.S,%.d,$(filter %.S,$(SRC)))

# include the C include depedencies
-include $(DEPS)

# calculate C include depedencies
%.d: %.c
	CC=$(CC) ./depend.sh `dirname $*` $(CFLAGS) $< >$@

# calculate Assembler include depencies
%.d: %.S
	CC=$(CC) ./depend.sh `dirname $*` $(CFLAGS) $< >$@

# If the no-op rule for source's wouldn't be here, make might not be
# able to run 'make clean' if a source file was missing.
$(SRC):
	@true

clean:	utilclean
	rm -f \
		$(TARGET_X86_64) \
		$(TARGET_MIPS32) \
		$(MIPS32OBJS) \
		$(X86_64OBJS) \
		$(DEPS) \
		*.map

.PHONY: all clean mips32 x86_64
