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

   $Id: trivial_io.c,v 1.24 2005/05/15 21:37:27 jaatroko Exp $
*/
#include "simulator.h"
#include "trivial_io.h"
#include "dummy.h"
#include "cpu.h"

static int cpu_irq;

void set_cpu_irq(int irq) {
  cpu_irq = irq;
}

/* port 0: cpu status */
int cpuinfo_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
  int cpunum;
  cpuinfo_t *cpu;

  if (dev == NULL || dev->realdevice == NULL) return -1;

  cpunum = dev->typecode & 0xff;
  cpu = (cpuinfo_t *) dev->realdevice;

  switch(addr) {
  case CPU_PORT_STATUS:
    {
      uint32_t status = cpu->error;

      /* mark CPU as running if it exists */
      if (cpunum < hardware->num_cpus)
        status += 1 << CPU_STATUS_RUNNING;

      if (cpu->irq_pending)
        status += 1 << CPU_STATUS_IRQ;

      *data = status;
      return 0;
    }

  case CPU_PORT_COMMAND:
    /* ignore command port on read (return 0) */
    *data = 0;
    return 0;

  default:
    /* fail */
    return 1;
  }


  return 0;
}

/* port 1: command port */
int cpuinfo_io_write(device_t *dev, uint32_t addr, uint32_t data) {
  int cpunum;
  cpuinfo_t *cpu;

  if (dev == NULL || dev->realdevice == NULL) return -1;
  cpunum = dev->typecode & 0xff;
  cpu = (cpuinfo_t *) dev->realdevice;

  switch(addr) {
  case CPU_PORT_STATUS:
    /* Ignore writes on status port */
    return 0;

  case CPU_PORT_COMMAND:
    if(data == CPU_COMMAND_GENERATE_IRQ) {
      cpu->error = 0;
      cpu->irq_pending = 1;
      return 0;
    }

    if(data == CPU_COMMAND_CLEAR_IRQ) {
      cpu->error = 0;
      cpu->irq_pending = 0;
      return 0;
    }

    cpu->error += 1 << CPU_STATUS_ICOMM;
    return 1;

  default:
    /* fail */
    return 1;
  }

  return 0;
}

/* Create CPU-info device. */
device_t *cpuinfo_create(int cpunum) {
  device_t *dev = NULL;
  cpuinfo_t *cpu_dev = NULL;

  if (cpunum < 0x00 || cpunum > 0xff)
    cpunum = 0;

  dev = (device_t *)smalloc(sizeof(device_t));
  cpu_dev = (cpuinfo_t *)smalloc(sizeof(cpuinfo_t));

  cpu_dev->irq_pending = 0;
  cpu_dev->error = 0;

  dev->typecode = TYPECODE_CPUINFO + cpunum;
  dev->realdevice = cpu_dev;
  memcpy(dev->vendor_string,"CPUINFO ", 8);
  dev->irq = cpu_irq;
  dev->io_length = IOLENGTH_CPUINFO;

  dev->io_read   = &cpuinfo_io_read;
  dev->io_write  = &cpuinfo_io_write;
  dev->update  = &cpuinfo_update;

  return dev;
}

/* Free the memory used by a CPU-info device. XXX */
void cpuinfo_destroy(device_t *cpu) {
  free(cpu->realdevice);
  free(cpu);
}

/* Raise the appropriate interrupts */
int cpuinfo_update(device_t *dev) {
  int cpunum;
  cpuinfo_t *cpu;

  if (dev == NULL || dev->realdevice == NULL) return -1;
  cpunum = dev->typecode & 0xff;
  cpu = (cpuinfo_t *) dev->realdevice;

  if (cpu->irq_pending) {
    RAISE_HW_INTERRUPT(cpunum, dev->irq);
  }

  return 0;
}

/* port 0: number of memory pages in the system */
int meminfo_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
  if (dev == NULL) return -1;
  if (addr != 0) return 1;

  /* operation succeeds even if data == NULL */
  if (data != NULL)
    *data = (uint32_t)(hardware->memory->num_pages);

  return 0;
}

device_t *meminfo_create() {
  device_t *dev = NULL;

  dev = (device_t *)smalloc(sizeof(device_t));
  dev->typecode = TYPECODE_MEMINFO;
  memcpy(dev->vendor_string,"MEMINFO ", 8);
  dev->irq = -1;
  dev->io_length = IOLENGTH_MEMINFO;

  dev->io_read   = &meminfo_io_read;
  dev->io_write  = &dummy_io_write;
  dev->update  = NULL;

  return dev;
}

/* port 0: milliseconds since system start
 * port 1: clock speed in Hz
 */
int rtc_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
  if (dev == NULL) return -1;
  if (addr != 0 && addr != 4) return 1;

  if (data != NULL) {
    if (addr == 0)
      *data = (uint32_t)((1000 * hardware->cycle_count) /
                 (uint64_t)(hardware->clockspeed));
    if (addr == 4)
      *data = hardware->clockspeed;
  }

  return 0;
}

/* Create rtc device descriptor */
device_t *rtc_create() {
  device_t *dev = NULL;

  dev = (device_t *)smalloc(sizeof(device_t));
  dev->typecode = TYPECODE_RTC;
  memcpy(dev->vendor_string,"SYS-RTC ", 8);
  dev->irq = -1;
  dev->io_length = IOLENGTH_RTC;

  dev->io_read   = &rtc_io_read;
  dev->io_write  = &dummy_io_write;
  dev->update = NULL;

  return dev;
}

/* port 0: software shutdown */
int shutdown_io_write(device_t *dev, uint32_t addr, uint32_t data) {
  if (dev == NULL) return -1;
  if (addr != 0) return 1;

  if (data == SHUTDOWN_MAGIC) {
    printf("Shutting down by software request\n");
    hardware->running = -1;
  } else if (data == HWCONSOLE_MAGIC) {
    printf("Dropping to command console by software request\n");
    hardware->running = 0;
  }

  return 0;
}

device_t *shutdown_create() {
  device_t *dev = NULL;

  dev = (device_t *)smalloc(sizeof(device_t));
  dev->typecode = TYPECODE_SHUTDOWN;
  memcpy(dev->vendor_string,"SHUTDOWN", 8);
  dev->irq = -1;
  dev->io_length = IOLENGTH_SHUTDOWN;

  dev->io_read   = &dummy_io_read;
  dev->io_write  = &shutdown_io_write;
  dev->update = NULL;

  return dev;
}
