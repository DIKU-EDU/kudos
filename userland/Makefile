# KUdos -- Buenos is University Educational Nutshell Operating System
#
# This is a Makefile for KUdos userland binaries.
#
#
# Copyright (C) 2003-2014  Juha Aatrokoski, Timo Lilja,
#        Leena Salmela, Teemu Takanen, Aleksi Virtanen
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

# $Id: Makefile,v 1.6 2005/05/09 00:05:44 jaatroko Exp $

# Add your _userland_ program sources to this variable:

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
