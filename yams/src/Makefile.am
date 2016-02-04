## Makefile for program src directory in YAMS.
##
## Copyright (C) 2002-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
##   Teemu Takanen, Aleksi Virtanen
##
## Copyright (C) 1996, 1997, 2001 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
## USA.

## Process this file with automake to produce Makefile.in.

AM_YFLAGS = -d
AM_CFLAGS = @WARNINGFLAGS@ @PTHREAD_CFLAGS@
AM_CPPFLAGS = @INCLUDEDIRS@
AM_LDFLAGS = @LIBDIRS@

bin_PROGRAMS = yams yamst

yams_SOURCES = yams.c version.c getopt.c getopt1.c getopt.h includes.h \
   cfg-parser.h misc.c misc.h device.h device.c opcodes.h cpu_defs.h cp0.c \
   simulator.c simulator.h cpu.h cpu.c memory.h memory.c \
   trivial_io.h trivial_io.c \
   hwcons.y hwcons-lex.l hwconsole.c \
   cfg-parser.y cfg-lexer.l cfg.c \
   dummy.h dummy.c io.h disk.h disk.c nic.h nic.c tty.h tty.c \
   cpuregs.h cp0.h hwconsole.h hwcons.h cfg.h \
   disasm.h disasm.c async_input.h async_input.c elf.h elf.c \
   plugio.h plugio.c gdb.h gdb.c

yamst_SOURCES = yamst.c

yams_LDADD = @LEXLIB@ @YAMSLIBS@ @PTHREAD_LIBS@ @LIBS@
yamst_LDADD = @LIBS@

## src/Makefile.am ends here
