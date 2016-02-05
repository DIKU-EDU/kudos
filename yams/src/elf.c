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

   $Id: elf.c,v 1.3 2010/11/23 12:20:26 jaatroko Exp $
*/

#include <stdio.h>
#include <string.h>
#include "includes.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include "elf.h"
#include "simulator.h"
#include "memory.h"

static int load_elf_file(FILE *f, uint32_t *entry_point);

/* Wrapper that handles file open and close */
int load_elf(char *fname, uint32_t *entry_point) {
    FILE *f;
    int ret;

    if ((f = fopen(fname, "rb")) == NULL) {
	printf("Couldn't open file [%s].\n", fname);
	return LOADELF_FAILURE;
    }

    ret = load_elf_file(f, entry_point);
    
    fclose(f);
    return ret;
}

/* Return 0 on failure, 1 if Not an ELF file, 2 on success */
static int load_elf_file(FILE *f, uint32_t *entry_point) {
    Elf32_Ehdr elfhdr;    
    Elf32_Phdr phdr;
    int i;
    long current_offset;
    uint32_t vaddr, size, offset, memsize;

    int physmemsize = hardware->memory->pagesize * hardware->memory->num_pages;
    uint8_t *physmem = hardware->memory->physmem;


    if (fread(&elfhdr, sizeof(elfhdr), 1, f) < 1) {
	return LOADELF_NOTELF;
    }
    if (memcmp(elfhdr.e_ident, ELF_MAGIC, 4)) {
	return LOADELF_NOTELF;
    }

    
    if (simulator_bigendian) {
	if (elfhdr.e_ident[EI_CLASS] != ELFCLASS32
	    || elfhdr.e_ident[EI_DATA] != ELFDATA2MSB
	    || simtoh16(elfhdr.e_machine) != EM_MIPS) {
	    printf("ELF: data is not MIPS 32-bit big-endian\n");
	    return LOADELF_FAILURE;
	}
    } else {
	if (elfhdr.e_ident[EI_CLASS] != ELFCLASS32
	    || elfhdr.e_ident[EI_DATA] != ELFDATA2LSB
	    || simtoh16(elfhdr.e_machine) != EM_MIPS_RS3_LE) {
	    printf("ELF: data is not MIPS 32-bit little-endian\n");
	    return LOADELF_FAILURE;
	}
    }

    if (simtoh32(elfhdr.e_version) != EV_CURRENT 
	|| elfhdr.e_ident[EI_VERSION] != EV_CURRENT) {
	printf("ELF: invalid version\n");
	return LOADELF_FAILURE;
    }

    if (simtoh16(elfhdr.e_type) != ET_EXEC) {
	printf("ELF: not an executable file\n");
	return LOADELF_FAILURE;
    }

    if (simtoh16(elfhdr.e_phnum) == 0) {
	printf("ELF: no program headers\n");
	return LOADELF_FAILURE;
    }

    *entry_point = simtoh32(elfhdr.e_entry);
    printf("ELF: entrypoint=#%.8" PRIx32 "\n", *entry_point);

    current_offset = simtoh32(elfhdr.e_phoff);
    fseek(f, current_offset, SEEK_SET);

    for (i = 0; i < simtoh16(elfhdr.e_phnum); i++) {
	if (fread(&phdr, sizeof(phdr), 1, f) < 1) {
	    printf("ELF: error reading program header\n");
	    return LOADELF_FAILURE;
	}

	switch (simtoh32(phdr.p_type)) {
	case PT_NULL:
	    printf("ELF: ignoring NULL program header\n");
	    break;
	case PT_NOTE:
	    printf("ELF: ignoring NOTE program header\n");
	    break;
	case PT_PHDR:
	    printf("ELF: ignoring PHDR program header\n");
	    break;
	case PT_LOAD:
	    vaddr = simtoh32(phdr.p_vaddr);
	    size = simtoh32(phdr.p_filesz);
	    memsize = simtoh32(phdr.p_memsz);
	    offset = simtoh32(phdr.p_offset);

	    printf("ELF: load: offset=#%.8" PRIx32, offset);
	    printf(" vaddr=#%.8" PRIx32, vaddr);
	    printf(" size=#%.8" PRIx32, size);
	    printf(" memsize=#%.8" PRIx32 "\n", memsize);
	    
	    /* Stuff must be loaded into unmapped segments. Since the
	     * I/O area begins at 0xB0000000, the valid memory range
	     * is 0x80000000 - 0xAFFFFFFF */
	    if (vaddr < 0x80000000 || vaddr >= 0xb0000000) {
		printf("ELF: invalid virtual address: #%.8" PRIx32,
			vaddr);
		return LOADELF_FAILURE;
	    }
	    if ((vaddr >= 0xa0000000 && vaddr+memsize > 0xb0000000)
		|| (vaddr < 0xa0000000 && vaddr+memsize > 0xa0000000)) {
		printf("ELF: program segment does not fit "\
			"into memory segment\n");
		return LOADELF_FAILURE;
	    }
	    /* convert to physical address */
	    if (vaddr < 0xa0000000)
		vaddr -= 0x80000000;
	    else
		vaddr -= 0xa0000000;

	    if (vaddr >= physmemsize) {
		printf("ELF: virtual address not in physical memory\n");
		return LOADELF_FAILURE;
	    }
	    if (vaddr+memsize > physmemsize) {
		printf("ELF: segment does not fit into physical memory\n");
		return LOADELF_FAILURE;
	    }

	    /* Zero the memory (ELF specifically requires this) */
	    memset(physmem+vaddr, 0, memsize);
	    /* Load the segment into memory */
	    fseek(f, offset, SEEK_SET);
	    if (fread(physmem+vaddr, size, 1, f) < 1) {
		printf("ELF: error loading program segment from file\n");
		return LOADELF_FAILURE;
	    }

	    break;
	case PT_DYNAMIC:
	    printf("ELF: detected dynamic linking information\n");
	    return LOADELF_FAILURE;
	case PT_INTERP:
	    printf("ELF: detected interpreter request\n");
	    return LOADELF_FAILURE;
    /*
     * Other program headers indicate an incompatible file *or* a file
     * with extra headers.  Just ignore for now.
     */
	}

	/* to the next program header */
	current_offset += simtoh16(elfhdr.e_phentsize); 
	fseek(f, current_offset, SEEK_SET);
    }

    return LOADELF_SUCCESS;
}
