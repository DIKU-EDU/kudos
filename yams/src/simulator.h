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

   $Id: simulator.h,v 1.24 2011/02/15 08:44:09 jaatroko Exp $
*/

#ifndef YAMS_SIMULATOR_H
#define YAMS_SIMULATOR_H

#include "includes.h"

#include <sys/types.h>
#include <arpa/inet.h>

#include "cpu.h"
#include "memory.h"
#include "misc.h"
#include "device.h"
#include "plugio.h"

typedef struct _hardware_t {
    memory_t *memory;

    cpu_t **cpus;
    int num_cpus;
    uint32_t clockspeed; /* in Hz */

    device_t *devices;
    int num_devices;

    uint64_t cycle_count;

    int running;
    uint32_t breakpoint;
} hardware_t;

extern hardware_t *hardware;

void simulator_create(int mem_pages, int num_cpus, int clock_speed, 
                      int cpu_irq);
void simulator_add_device(device_t *dev);
void simulator_add_mmap(pluggable_device_t *dev);
void simulator_init();
void simulator_run(uint64_t stop_at_cycle);




/* Byte-swap functions to support little-endian simulator: */

extern int simulator_bigendian;

#ifdef WORDS_BIGENDIAN

/* These may not be extremely efficient, but fortunately the case
   where they are needed (simulating a little-endian machine on a
   big-endian host) is not very common
*/
static inline uint16_t swap_bytes_16(uint16_t x) {
    return ((x & 0xff00) >> 8)
	| ((x & 0x00ff) << 8);
}
static inline uint32_t swap_bytes_32(uint32_t x) {
    return ((x & 0xff000000) >> 24)
	| ((x & 0x00ff0000) >> 8)
	| ((x & 0x0000ff00) << 8)
	| ((x & 0x000000ff) << 24);
}

#define tofrombe16(x) (x)
#define tofrombe32(x) (x)
#define tofromle16(x) swap_bytes_16(x)
#define tofromle32(x) swap_bytes_32(x)

#else

#define swap_bytes_16(x) ntohs(x) 
#define swap_bytes_32(x) ntohl(x)
#define tofrombe16(x) swap_bytes_16(x)
#define tofrombe32(x) swap_bytes_32(x)
#define tofromle16(x) (x)
#define tofromle32(x) (x)

#endif /* WORDS_BIGENDIAN */

#define simtoh16(x) (simulator_bigendian ? tofrombe16(x) : tofromle16(x))
#define htosim16(x) simtoh16(x)
#define simtoh32(x) (simulator_bigendian ? tofrombe32(x) : tofromle32(x))
#define htosim32(x) simtoh32(x)

#endif /* YAMS_SIMULATOR_H */
