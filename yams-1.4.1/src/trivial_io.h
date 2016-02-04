/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: trivial_io.h,v 1.12 2005/03/08 12:57:01 lsalmela Exp $
*/
#ifndef YAMS_TRIVIAL_IO_H
#define YAMS_TRIVIAL_IO_H

#include "device.h"

#define SHUTDOWN_MAGIC 0x0badf00d
#define HWCONSOLE_MAGIC 0xdeadc0de

#define IOLENGTH_RTC          8           /* bytes */
#define IOLENGTH_SHUTDOWN     4
#define IOLENGTH_CPUINFO      8
#define IOLENGTH_MEMINFO      4

#define CPU_PORT_STATUS 0x00
#define CPU_PORT_COMMAND 0x04
#define CPU_COMMAND_GENERATE_IRQ 0x00
#define CPU_COMMAND_CLEAR_IRQ 0x01
#define CPU_STATUS_RUNNING 0
#define CPU_STATUS_IRQ 1
#define CPU_STATUS_ICOMM 31

#define CPU_IRQ_DEFAULT 4

int cpuinfo_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int cpuinfo_io_write(device_t *dev, uint32_t addr, uint32_t data);
int meminfo_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int rtc_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int shutdown_io_write(device_t *dev, uint32_t addr, uint32_t data);

device_t *rtc_create();
device_t *shutdown_create();
device_t *cpuinfo_create(int cpunum);
void cpuinfo_destroy(device_t *cpu);
device_t *meminfo_create();

void set_cpu_irq(int irq);
int cpuinfo_update(device_t *dev);

/* The CPU-info real device */
typedef struct {
    int error;
    int irq_pending;
} cpuinfo_t;

#endif /* YAMS_TRIVIAL_IO_H */


