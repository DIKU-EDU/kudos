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

   $Id: disk.h,v 1.5 2002/11/11 15:28:29 tlilja Exp $
*/
#ifndef YAMS_DISK_H
#define YAMS_DISK_H

/* IO port names */
#define PORT_STATUS  0
#define PORT_COMMAND 4
#define PORT_DATA    8
#define PORT_TSECTOR 12
#define PORT_DMAADDR 16

/* Status/error bits */
#define STATUS_MASK    0x0f
#define STATUS_IRQMASK 0x0c
#define STATUS_RBUSY 1
#define STATUS_WBUSY 2
#define STATUS_RIRQ  4
#define STATUS_WIRQ  8
#define ERROR_MASK  0xf8000000
#define ERROR_ISECT 0x08000000
#define ERROR_IADDR 0x10000000
#define ERROR_ICOMM 0x20000000
#define ERROR_EBUSY 0x40000000
#define ERROR_ERROR 0x80000000

/* Command codes */
#define COMMAND_READ         1
#define COMMAND_WRITE        2
#define COMMAND_RESETRIRQ    3
#define COMMAND_RESETWIRQ    4
#define COMMAND_GETSEC       5
#define COMMAND_GETSECSIZE   6
#define COMMAND_GETSECPERCYL 7
#define COMMAND_GETROT       8
#define COMMAND_GETSEEK      9

/* disk operating modes */
#define MODE_IDLE     0
#define MODE_READING1 1
#define MODE_READING2 2
#define MODE_WRITING1 3
#define MODE_WRITING2 4

/* disk device */

#define IRQ_DISK 3
#define IOLENGTH_DISK 20

typedef struct {
    char *imagefilename;
    FILE *image;

    int numsectors, sectorsize, numcylinders, current_cylinder;
    int64_t time_rot, time_fullseek; /* in clock cycles */

    /* Internal stuff */
    uint32_t status_word, transfer_sector, new_transfer_sector,
	transfer_addr, new_transfer_addr, data_port;
    uint64_t next_interest;
    int mode, irqcpu;
} diskdevice_t;



/* Allocates memory for a disk device */
device_t *disk_create();
void disk_destroy(device_t *disk);

/* places the initialized data in disk 
 * returns:
 * 0 - OK
 * 1 - file too small to hold disk
 * 2 - unable to create file
 * 3 - invalid sectorsize/numsectors
 */
int disk_init(char *filename, int sectorsize, int numsectors,
	      device_t *disk);

/* returns: 0 - OK, 1 - invalid parameters, 2 - parameter already set 
 * value -1 for a parameter is ignored */
int disk_setparams(int numcylinders, int time_rot, int time_fullseek,
		   device_t *disk);


int disk_io_write(device_t *dev, uint32_t addr, uint32_t data);
int disk_io_read(device_t *dev, uint32_t addr, uint32_t *data);
int disk_update(device_t *dev);

#endif /* YAMS_DISK_H */
