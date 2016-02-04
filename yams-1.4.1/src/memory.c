/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2010 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

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

   $Id: memory.c,v 1.36 2010/11/23 12:20:26 jaatroko Exp $
*/

#include "includes.h"
#include <assert.h>
#include <netinet/in.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "simulator.h"

memory_t * memory_create(uint32_t pagesize, uint32_t num_pages) {
    memory_t *memory;

    memory               = (memory_t *) smalloc(sizeof(memory_t));
    memory->pagesize     = pagesize; 
    memory->num_pages    = num_pages;
    memory->physmem      = smalloc(num_pages * pagesize);
    memory->io_descrarea = smalloc(IO_DESCRAREA_LENGTH);
    memory->kernel_paramarea = smalloc(KERNEL_PARAMAREA_LENGTH);

    memset(memory->physmem, 0, memory->pagesize * memory->num_pages);
    memset(memory->io_descrarea, 0, IO_DESCRAREA_LENGTH);
    memset(memory->kernel_paramarea, 0, KERNEL_PARAMAREA_LENGTH);

    memory->mmap = NULL;
    memory->num_mmap = 0;

    return memory;
}

/* does not check TLB exceptions so make sure they do not 
 * occur when calling this ;) (e.g. by doing memory load/store _before_)
 */
uint32_t phys_addr(uint32_t vaddr, cpu_t *cpu) {
    uint32_t paddr;

    if (vaddr >= 0xC0000000 || vaddr < 0x80000000) {
	/* kernel/supervisor/user mapped */
	if (tlb_translate(cpu, vaddr, &paddr, MEM_LOAD) != NoException)
	    return 0xffffffff;
    } else if (vaddr >= 0xA0000000) {
	/* kernel unmapped uncached */
	paddr = vaddr - 0xA0000000;
    } else {
	/* kernel unmapped @0x80000000 */
	paddr = vaddr - 0x80000000;
    }

    return paddr;
}

exception_t mem_read(memory_t * mem, uint32_t addr, void * buf, 
                     int size, cpu_t *cpu) {
    int ioread = 0;
    uint8_t *readfrom;
    exception_t exc;
    pluggable_device_t *plugio = NULL;

    assert(size == 0 || size == 1 || size == 2 || size == 4);
 
    if (size != 0 && (addr & (size - 1)))
	return AddressLoad; /* alignment error */

    readfrom = (uint8_t*)mem->physmem; /* default to normal memory read */

    /* Segment translation */
    if (addr >= 0xC0000000) {
	/* 0xE0000000: kernel mapped
	 * 0xC0000000: supervisor mapped
	 * treat supervisor mode as kernel mode */
	if (!CP0_KERNEL_MODE(cpu)) return AddressLoad;
	exc = tlb_translate(cpu, addr, &addr, MEM_LOAD);
	if (exc != NoException)
	    return exc;
    } else if (addr >= 0xA0000000) { /* kernel unmapped uncached */
	if (!CP0_KERNEL_MODE(cpu)) return AddressLoad;

	/* IO area */
	if (addr >= 0xB0000000) {
	    ioread = 3;

	    /* Check MMAP areas */
	    for (plugio=mem->mmap; plugio != NULL; plugio = plugio->mmap_next)
		if (addr >= plugio->mmap_base 
		    && addr < plugio->mmap_base + plugio->mmap_size) {
		    ioread = 5;
		    break;
		}
	}

	/* IO descriptor area */
	if (addr >= IO_DESCRAREA_BASE 
	    && addr < IO_DESCRAREA_BASE + IO_DESCRAREA_LENGTH)
	    ioread = 1;

	/* Kernel boot string area */
	else if (addr >= KERNEL_PARAMAREA_BASE 
	    && addr < KERNEL_PARAMAREA_BASE + KERNEL_PARAMAREA_LENGTH)
	    ioread = 4;

	/* IO device area */
	else if (addr >= mem->io_area_base
		 && addr < mem->io_area_base + mem->io_area_length)
	    ioread = 2;

	/* "normal" area */
	else if (!ioread)
	    addr -= 0xA0000000;

    } else if (addr >= 0x80000000) { /* kernel unmapped */
	if (!CP0_KERNEL_MODE(cpu)) return AddressLoad;
	addr -= 0x80000000;
    } else { /* user mapped */
	exc = tlb_translate(cpu, addr, &addr, MEM_LOAD);
	if (exc != NoException)
	    return exc;
    }


    /* Unused IO area read, return 0 */
    if (ioread == 3) {
	if (size != 0) memset(buf, 0, size);
	return NoException;
    }
    /* IO descriptor area read */
    if (ioread == 1) {
	addr -= IO_DESCRAREA_BASE;
	readfrom = (uint8_t*)mem->io_descrarea;
    }
    /* Kernel parameter area read */
    if (ioread == 4) {
	addr -= KERNEL_PARAMAREA_BASE;
	readfrom = (uint8_t*)mem->kernel_paramarea;
    }
    /* MMAP area read */
    if (ioread == 5) {
	if (size != 0) {
	    plugio_mmap_read(plugio, addr - plugio->mmap_base, buf, size);
	    if (size == 2) *((uint16_t*)buf) = simtoh16(*((uint16_t*)buf));
	    if (size == 4) *((uint32_t*)buf) = simtoh32(*((uint32_t*)buf));
	}
	return NoException;
    }
    /* IO device area read */
    if (ioread == 2) {
	uint32_t word, i;
	int result;

	i = (addr - mem->io_area_base) >> 2;
	result = mem->io_device_read[i](mem->io_devices[i],
					((addr - mem->io_devices[i]->io_base) 
					 & 0xfffffffc),
					&word);
	assert(result == 0); /* Caller's fault if nonzero */

	switch(size) {
	case 4:
	    *((uint32_t*)buf) = word;
	    break;
	case 2:
	    if ((addr & 3) == 0) { /* want first halfword */
		if (simulator_bigendian) /* first halfword is high */
		    word >>= 16;
	    } else { /* want second halfword */
		if (!simulator_bigendian) /* second halfword is high */
		    word >>= 16;
	    }
	    *((uint16_t*)buf) = (uint16_t)(word & 0xffff);
	    break;
	case 1:
	    if (simulator_bigendian) {
		word >>= 8*(3 - (addr & 3)); /* select byte */
	    } else {
		word >>= 8*(addr & 3); /* select byte */
	    }
	    *((uint8_t*)buf) = (uint8_t)(word & 0xff);
	}

	return NoException;
    }

   
    /* Out of memory bounds, normal memory */
    if (!ioread  &&  addr >= mem->num_pages * mem->pagesize)
	return BusErrorData; /* Caller checks for Instr case */

    /* normal read */
    switch(size) {
    case 1:
	*((uint8_t*)buf)
            = *(readfrom + addr);
	break;
    case 2:
	*((uint16_t*)buf)
            = simtoh16(*((uint16_t*)(readfrom + addr)));
	break;
    case 4:
	*((uint32_t*)buf)
            = simtoh32(*((uint32_t*)(readfrom + addr)));
    }

    return NoException;
}

exception_t mem_write(memory_t *mem, uint32_t addr, void *buf, 
                      int size, cpu_t *cpu) {
    int iowrite = 0;
    exception_t exc;
    pluggable_device_t *plugio = NULL;

    assert(size == 0 || size == 1 || size == 2 || size == 4);

    if (size != 0 && (addr & (size - 1)))
	return AddressStore; /* alignment error */

   
    /* Segment translation */
    if (addr >= 0xC0000000) {
	/* 0xE0000000: kernel mapped
	 * 0xC0000000: supervisor mapped
	 * treat supervisor mode as kernel mode */
	if (!CP0_KERNEL_MODE(cpu)) return AddressLoad;
	exc = tlb_translate(cpu, addr, &addr, MEM_STORE);
	if (exc != NoException)
	    return exc;
    } else if (addr >= 0xA0000000) {/* kernel unmapped uncached */
	if (!CP0_KERNEL_MODE(cpu)) return AddressStore;

	/* IO area */
	if (addr >= 0xB0000000) {
	    iowrite = 3;
	    
	    /* Check MMAP areas */
	    for (plugio=mem->mmap; plugio != NULL; plugio = plugio->mmap_next)
		if (addr >= plugio->mmap_base 
		    && addr < plugio->mmap_base + plugio->mmap_size) {
		    iowrite = 5;
		    break;
		}
	}

	/* IO descriptor area */
	if (addr >= IO_DESCRAREA_BASE 
	    && addr < IO_DESCRAREA_BASE + IO_DESCRAREA_LENGTH)
	    iowrite = 1;

	/* IO device area */
	else if (addr >= mem->io_area_base
		 && addr < mem->io_area_base + mem->io_area_length)
	    iowrite = 2;

	/* "normal" area */
	else if (!iowrite)
	    addr -= 0xA0000000;

    } else if (addr >= 0x80000000) { /* kernel unmapped */
	if (!CP0_KERNEL_MODE(cpu)) return AddressStore;
	addr -= 0x80000000;
    } else { /* user mapped */
	exc = tlb_translate(cpu, addr, &addr, MEM_STORE);
	if (exc != NoException)
	    return exc;
    }


    /* Unused IO area write, do nothing */
    if (iowrite == 3) return NoException;
    /* IO descriptor area is read-only but does not cause exceptions */
    if (iowrite == 1) return NoException;
    /* MMAP area write */
    if (iowrite == 5) {
	uint32_t d = *((uint32_t*)buf);
	uint16_t *d16 = (void*)&d; /* avoid type-punning wrnings with void */
	if (size != 0) {
	    if (size == 2) *d16 = htosim16(*d16);
	    if (size == 4) d = htosim32(d);
	    plugio_mmap_write(plugio, addr - plugio->mmap_base, &d, size);
	}
	return NoException;
    }
    /* Write to IO devices */
    if (iowrite == 2) {
	uint32_t word, i;
	int result;

	switch(size) {
	case 4:
	    word = *((uint32_t*)buf);
	    break;
	case 2:
	    word = (uint32_t)(*((uint16_t*)buf));
	    if ((addr & 3) == 0) { /* write to first halfword */
		if (simulator_bigendian) /* first halfword is high */
		    word <<= 16;
	    } else { /* write to second halfword */
		if (!simulator_bigendian) /* second halfword is high */
		    word <<= 16;
	    }
	    break;
	case 1:
	    word = (uint32_t)(*((uint8_t*)buf));
	    if (simulator_bigendian) {
		word <<= 8*(3 - (addr & 3)); /* write the right byte */
	    } else {
		word <<= 8*(addr & 3); /* write the right byte */
	    }
	    break;
	default:
	    fprintf(stderr, "internal error in iowrite\n");
	    exit(1);
	}

	if (size != 0) {
	    i = (addr - mem->io_area_base) >> 2;
	    result = mem->io_device_write[i](mem->io_devices[i],
					     (addr 
					      - mem->io_devices[i]->io_base)
					     & 0xfffffffc,
					     word);
	    assert(result == 0); /* Caller's fault if nonzero */
	}

	return NoException;
    }


    /* Out of memory bounds */
    if (addr >= mem->num_pages * mem->pagesize)
	return BusErrorData; /* Caller checks for Instr case */

    /* normal write */
    switch(size) {
    case 1:
	*((uint8_t*)mem->physmem + addr)
            = *((uint8_t*)buf);
	break;
    case 2:
	*((uint16_t*)((uint8_t*)mem->physmem + addr))
            = htosim16(*((uint16_t*)buf));
	break;
    case 4:
	*((uint32_t*)((uint8_t*)mem->physmem + addr))
            = htosim32(*((uint32_t*)buf));
    }

    return NoException;
}



int mem_dread(memory_t *mem, uint32_t addr, void *buf, int size) {

    assert(size == 1 || size == 2 || size == 4);

    /* is there need to check the alignment? */
#if 1
    if (addr & (size - 1))
	return 1; /* alignment error */
#endif   

    /* Out of memory bounds */
    if (addr > mem->num_pages * mem->pagesize - size)
	return 2;

    switch(size) {
    case 1:
	*((uint8_t*)buf)
            = *(((uint8_t*) mem->physmem) + addr);
	break;
    case 2:
	*((uint16_t*)buf)
            = simtoh16(*((uint16_t*)((uint8_t*)mem->physmem + addr)));
	break;
    case 4:
	*((uint32_t*)buf)
            = simtoh32(*((uint32_t*)((uint8_t*)mem->physmem + addr)));
    }

    return 0;
}

int mem_dwrite(memory_t *mem, uint32_t addr, void *buf, int size) {
    assert(size == 1 || size == 2 || size == 4);

    /* is there need to check the alignment? */
#if 1
    if (addr & (size - 1))
	return 1; /* alignment error */
#endif

    /* Out of memory bounds */
    if (addr > mem->num_pages * mem->pagesize - size)
	return 2;

    switch(size) {
    case 1:
	*((uint8_t*)mem->physmem + addr)
            = *((uint8_t*)buf);
	break;
    case 2:
	*((uint16_t*)((uint8_t*)mem->physmem + addr))
            = htosim16(*((uint16_t*)buf));
	break;
    case 4:
	*((uint32_t*)((uint8_t*)mem->physmem + addr))
            = htosim32(*((uint32_t*)buf));
    }

    return 0;
}




int mem_store_direct(memory_t *mem, uint32_t addr, 
		     uint32_t length, void *data) {
    uint32_t size = mem->num_pages * mem->pagesize; 

    if(addr+length > size) {
	/* write over end of memory */
	return 1;
    }

    memcpy((char*)mem->physmem + addr, data, length);
    
    return 0;
} 

int mem_read_direct(memory_t *mem, uint32_t addr, 
		    uint32_t length, void *data) {
    uint32_t size = mem->num_pages * mem->pagesize; 

    if(addr+length > size) {
	/* read over end of memory */
	return 1;
    }

    memcpy(data, (char*)mem->physmem + addr, length);
    
    return 0;
} 
