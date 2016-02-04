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

   $Id: plugio.h,v 1.2 2010/11/24 13:26:30 jaatroko Exp $
*/

#include "device.h"

#ifndef YAMS_PLUGIO_H
#define YAMS_PLUGIO_H

#define PLUGIO_FLAG_LAST       0x80000000
#define PLUGIO_FLAG_ASYNC      0x40000000
#define PLUGIO_FLAG_WORDSLE    0x20000000

#define PLUGIO_CMD(x) (((x) & 0xff00) >> 8)
#define PLUGIO_CPU(x) ((x) & 0xff)
#define PLUGIO_TAG(x) (((x) & 0xff0000) >> 16)
#define PLUGIO_MAKECMD(flags, tag, cmd, cpu) \
	((flags) | (cpu) | ((tag) << 16) | ((cmd) << 8))

#define PLUGIO_CMD_INIT     1
#define PLUGIO_REPLY_DEVICE 2
#define PLUGIO_CMD_MMAP     3

#define PLUGIO_CMD_PORTR   10
#define PLUGIO_CMD_PORTW   11
#define PLUGIO_CMD_DATAR   12
#define PLUGIO_CMD_DATAW   13
#define PLUGIO_CMD_ADELAY  14
#define PLUGIO_CMD_ALARM   15

#define PLUGIO_REPLY_OK     100
#define PLUGIO_REPLY_WORD   101
#define PLUGIO_REPLY_DATA   102
#define PLUGIO_REPLY_DELAY  103
#define PLUGIO_REPLY_IRQ    104
#define PLUGIO_REPLY_CPUIRQ 105
#define PLUGIO_REPLY_CLIRQ  106
#define PLUGIO_REPLY_DMAW   107
#define PLUGIO_REPLY_DMAR   108
#define PLUGIO_REPLY_TIMER  109


typedef struct _pluggable_device_t {
    struct _pluggable_device_t *mmap_next; /* for MMAP handling */
    struct _pluggable_device_t *next; /* for fd,tag -> dev lookup */

    int tag, async;

    int fd;

    uint32_t mmap_size, mmap_base;

    int irq_processor, irq_pending;

    uint64_t delayed_effect, timer;
} pluggable_device_t;


int plugio_init(int domain, char *name, int port, int listen,
	       int irq, int async, char *options);
void plugio_destroy(device_t *plugio_dev);

int plugio_mmap(pluggable_device_t *dev, uint32_t addr);

int plugio_mmap_read(pluggable_device_t *dev, uint32_t addr,
		    void *buf, int size);
int plugio_mmap_write(pluggable_device_t *dev, uint32_t addr,
		     void *buf, int size);

int plugio_io_write(device_t *dev, uint32_t addr, uint32_t data);
int plugio_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int plugio_update(device_t *dev);


#endif /* YAMS_PLUGIO_H */
