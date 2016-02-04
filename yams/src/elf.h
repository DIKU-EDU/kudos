/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2010 Juha Aatrokoski, Timo Lilja, Leena Salmela,
     Teemu Takanen, Aleksi Virtanen

   Copyright (C) 1992, 1995, 1996, 1997-1999, 2000-2002
                  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: elf.h,v 1.2 2010/11/23 12:20:26 jaatroko Exp $
*/

#ifndef YAMS_ELF_H
#define YAMS_ELF_H

#include "includes.h"


#define LOADELF_FAILURE 0
#define LOADELF_NOTELF 1
#define LOADELF_SUCCESS 2
int load_elf(char *fname, uint32_t *entry_point);


/* This stuff directly from TIS/ELF 1.2 specification */

#define EI_NIDENT 16
#define ET_EXEC 2
#define EM_MIPS 8
#define EM_MIPS_RS3_LE  10

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#define EV_CURRENT 1

#define ELF_MAGIC "\177ELF"

/* ELF file header. The fields are aligned on data type sizes so no
 * need for the (non-portable) packed-attribute. 
 */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* ELF program header */
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

#endif /* YAMS_ELF_H */
