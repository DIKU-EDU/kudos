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

   $Id: device.h,v 1.5 2002/11/20 12:35:40 ttakanen Exp $
*/
#ifndef YAMS_DEVICE_H
#define YAMS_DEVICE_H

#define TYPECODE_MEMINFO      0x101
#define TYPECODE_RTC          0x102
#define TYPECODE_SHUTDOWN     0x103
#define TYPECODE_TTY          0x201
#define TYPECODE_DISK         0x301
#define TYPECODE_NIC          0x401
#define TYPECODE_CPUINFO      0xC00

#include "includes.h"

typedef struct _device_t {
  struct _device_t *next;
  
  uint32_t typecode;
  char vendor_string[8];
  uint32_t io_base;
  uint32_t irq;
  uint32_t io_length;
  
  /* This is a pointer to a structure containing all the parameters
   * for a specified device. e.g. for disk devices the structure
   * contains the image file name and handles etc. This may be left
   * NULL for the simpler devices which do not need stored
   * variables. This is initialized when the device (structure) is
   * created.
   *
   * The io_read, io_write and update functions can then access all
   * needed variables by typecasting,
   * e.g. (diskdevice_t*)(dev->realdevice)
   */
  void *realdevice;
  
  int (*io_write)(struct _device_t *dev, uint32_t addr, uint32_t data);
  int (*io_read)(struct _device_t *dev, uint32_t addr, uint32_t *data);
  int (*update)(struct _device_t *dev);
} device_t;

void device_set_irq(device_t *dev, uint32_t irq);
void device_set_vendor(device_t *dev, char *vendor);

#endif /* YAMS_DEVICE_H */


