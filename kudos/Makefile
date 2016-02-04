# KUdos -- KUdos is Københavns Universitet Diskrete Operations System
# Main Makefile for KUDOS OS
#
# Copyright (C) 2003-2014  Juha Aatrokoski, Timo Lilja,
#        Leena Salmela, Teemu Takanen, Aleksi Virtanen,
#        Philip Meulengracht.
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#  3. The name of the author may not be used to endorse or promote
#     products derived from this software without specific prior
#     written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This Makefile skeleton is excerpted from the paper
# _Recursive Make Considered Harmful_ by Peter Miller
# <URL:http://www.tip.net.au/~millerp/rmch/recu-make-cons-harm.html>

# $Id: Makefile,v 1.36 2012/02/03 12:44:38 tlilja Exp $

## User configurable variables
MODULES  := init kernel lib drivers proc vm fs util lib/gcc 
ARCHMIPS := init/mips kernel/mips vm/mips lib/mips drivers/mips proc/mips
ARCH64   := init/x86_64 kernel/x86_64 vm/x86_64 lib/x86_64 drivers/x86_64 proc/x86_64
#CHANGEDFLAGS := -DCHANGED_1 -DCHANGED_2 -DCHANGED_3 -DCHANGED_4 -DCHANGED_5
PHASE        := 1

#IncludePaths
MIPS_INC = -I./drivers/mips/ -I./lib/mips/ -I./kernel/mips/ -I./vm/mips
X64_INC = -I./drivers/x86_64/ -I./lib/x86_64/ -I./kernel/x86_64/ -I./vm/x86_64

## Below this point, you shouldn't have to change anything.
TARGETMIPS := kudos-mips
TARGET64   := kudos64
UTILTARGET := util/tfstool


# Compiler and tar configuration
CC      := mips-elf-gcc
AS      := mips-elf-as
LD      := mips-elf-ld

CFLAGS  += -g -G0 -O2 -I. -Wall -W -Werror $(CHANGEDFLAGS)
LDFLAGS := --script=ld.script --cref -G0 -Map kudos-mips.map
ASFLAGS := -gstabs+ -I. -Wa,-mips32 $(CHANGEDFLAGS)

CC64	:= x86_64-elf-gcc
AS64	:= x86_64-elf-as
LD64	:= x86_64-elf-ld

CFLAGS64  += -O2 -I. -g -fverbose-asm -Wall -Werror -Wno-attributes -m64 -mcmodel=small -mno-red-zone -finline-functions -fno-stack-protector -ffreestanding -D SMALL_ENDIAN $(CHANGEDFLAGS)
LDFLAGS64 := --script=ld64.script -z max-page-size=0x1000 -Map kudos64.map
ASFLAGS64 := -I.

# -G0 is needed to avoid GP optimization (LD won't link if it is used)
GTAR    := tar

# Each module adds to this
SRC     :=
MIPSSRC := 
X64SRC  :=

all: $(TARGETMIPS) $(TARGET64) $(UTILTARGET)

install: $(TARGET64)
	mount /dev/sda6
	cp -f ./$(TARGET64) $(HOME)/kos/boot/kUdOS
	umount $(HOME)/kos

copy:
	dd if=$(IMG) of=/dev/sda7

# include the description for each module
include $(patsubst %, %/module.mk, $(MODULES))
include $(patsubst %, %/module.mk, $(ARCHMIPS))
include $(patsubst %, %/module.mk, $(ARCH64))

# determine the object files
MIPSOBJS :=  $(patsubst %.s,%.mips_o,$(filter %.s,$(SRC))) \
	 $(patsubst %.S,%.mips_o,$(filter %.S,$(SRC))) \
         $(patsubst %.c,%.mips_o,$(filter %.c,$(SRC))) \
         $(patsubst %.s,%.mips_o,$(filter %.s,$(MIPSSRC))) \
	 $(patsubst %.S,%.mips_o,$(filter %.S,$(MIPSSRC))) \
         $(patsubst %.c,%.mips_o,$(filter %.c,$(MIPSSRC)))

X64OBJS := $(patsubst %.s,%.x86_o,$(filter %.s,$(X64SRC))) \
	$(patsubst %.S,%.x86_o,$(filter %.S,$(X64SRC))) \
        $(patsubst %.c,%.x86_o,$(filter %.c,$(X64SRC))) \
	$(patsubst %.s,%.x86_o,$(filter %.s,$(SRC))) \
	$(patsubst %.S,%.x86_o,$(filter %.S,$(SRC))) \
	$(patsubst %.c,%.x86_o,$(filter %.c,$(SRC)))

# link the program

# For some reason the order of the object file matters; the startup
# code (kernel/boot.o) must come before other files.
$(TARGETMIPS): $(MIPSOBJS) 
	$(LD) -o $@ $(LDFLAGS) $^

# Compile rule for assembler source
%.mips_o: %.s
	$(AS) -o $@ -mips32 $<

%.mips_o: %.S
	$(CC) -o $@ $(ASFLAGS) -c $<

# Compile rule for C source
%.mips_o: %.c
	$(CC) $(MIPS_INC) -o $@  $(CFLAGS) -c $<

$(TARGET64): $(X64OBJS)
	$(LD64) -o $@ $(LDFLAGS64) $^

# Compile rule for assembler source
%.x86_o: %.s
	$(AS64) -o $@ $<

%.x86_o: %.S
	$(CC64) -o $@ $(ASFLAGS64) -c $<

# Compile rule for C source
%.x86_o: %.c
	$(CC64) $(X64_INC) -o $@  $(CFLAGS64) -c $<


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

doc: $(SRC)
	doc++ -t -o doc/code/buenos.tex ${SRC}
	doc++ -t -s -o doc/code/buenos-src.tex ${SRC}

# TAGS
ctags:
	find . -type f -name '*.[ch]' | xargs ctags

etags:
	find . -type f -name '*.[ch]' | xargs etags

tags: ctags etags

# If the no-op rule for source's wouldn't be here, make might not be
# able to run 'make clean' if a source file was missing.
$(SRC):
	@true

# Builds the kudos.tar.gz to the parent directory!
# This target includes docs. Do not use this for submission
kudos.tar.gz: $(SRC)
	(cd ..; find kudos -type f | grep -Ev '[.]d$$' \
	        | grep -Ev 'no-dist|/CVS/|[.]cvsignore' \
                | xargs $(GTAR) cvf - | gzip >$@)

# This is the target to build the submission archive.
submit-archive: $(SRC)
	(cd ..; $(GTAR) cf - kudos | gzip >submit-$(PHASE).tar.gz)

clean:	utilclean
	rm -f $(TARGET64) $(TARGETMIPS) $(MIPSOBJS) $(X64OBJS) *~ */*~ kudos32.map tags TAGS

real-clean: clean
	rm -f $(DEPS)

.PHONY: clean
.PHONY: doc
.PHONY: submit-archive kudos.tar.gz

