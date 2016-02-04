/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: memory.h,v 1.17 2005/06/05 14:20:05 jaatroko Exp $
*/
#ifndef MEMORY_H
#define MEMORY_H

#include <sys/types.h>
#include "cpu.h"
#include "plugio.h"

#define SIMULATOR_PAGESIZE 4096
#define IO_AREA_BASE_ADDRESS 0xb0008000
#define IO_DESCRAREA_BASE 0xB0000000
#define IO_DESCRAREA_LENGTH 4096
#define KERNEL_PARAMAREA_BASE 0xB0001000
#define KERNEL_PARAMAREA_LENGTH 4096

#define STARTUP_PC 0x80010000
#define STARTUP_REAL_ADDRESS 0x00010000

struct _device_t;

typedef struct _memory_t{
    void *physmem;
    int pagesize;
    int num_pages;
    
    void *io_descrarea; /* io descriptors stored in this buffer */
    void *kernel_paramarea;
    
    uint32_t io_area_base;
    uint32_t io_area_length;
    
    /* dynamic lookup tables for memory mapped devices */
    int (**io_device_write)(struct _device_t *dev, 
			    uint32_t addr, 
			    uint32_t data);
    int (**io_device_read)(struct _device_t *dev, 
			   uint32_t addr, 
			   uint32_t *data);
    struct _device_t **io_devices;

    /* A chain of PLUGIO devices to check for MMAP access */
    pluggable_device_t *mmap;
    int num_mmap;
} memory_t;

/* Create simulated memory, return allocated info struct */
memory_t * memory_create(uint32_t pagesize, uint32_t num_pages);

/* Translates a virtual address to a physical address
 * does not check TLB exceptions so make sure they do not 
 * occur when calling this ;) (e.g. by doing memory load/store _before_)
 */
uint32_t phys_addr(uint32_t vaddr, cpu_t *cpu);

/* Read a byte, halfword or word from memory. Return exception number. */
exception_t mem_read(memory_t *mem, uint32_t addr, void *buf, 
                     int size, cpu_t *cpu);

/* Write a byte, halfword or word to memory. Return exception number. */
exception_t mem_write(memory_t *mem, uint32_t addr, void *buf, 
                      int size, cpu_t *cpu);


/* byte/halfword/word operations without translations */
int mem_dread(memory_t *mem, uint32_t addr, void *buf, int size);
int mem_dwrite(memory_t *mem, uint32_t addr, void *buf, int size);


/* Read and write directly to the memory. No translation/conversion 
   is done.
*/
int mem_store_direct(memory_t *mem, uint32_t addr, 
		     uint32_t length, void *data);
int mem_read_direct(memory_t *mem, uint32_t addr, 
		    uint32_t length, void *data);

#endif /* MEMORY_H */
