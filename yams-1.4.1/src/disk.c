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

   $Id: disk.c,v 1.19 2005/05/02 06:51:35 lsalmela Exp $
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "simulator.h"
#include "io.h"
#include "disk.h"

extern hardware_t *hardware;

/* conversion macro */
#define MSEC2TICKS_64(n) \
  ((int64_t)(((uint64_t)(n)) * hardware->clockspeed / 1000))
#define TICKS2MSEC_32(n) \
  ((uint32_t)((1000 * (uint64_t)(n)) / (uint64_t)(hardware->clockspeed)))


device_t *disk_create() {
    device_t *dev;
    diskdevice_t *ddev;
    
    dev = (device_t *) smalloc(sizeof(device_t));
    ddev = (diskdevice_t *) smalloc(sizeof(diskdevice_t));
    memset(ddev, 0, sizeof(diskdevice_t));

    /* NULL might not be 0 ;) */
    ddev->image = NULL;
    ddev->imagefilename = NULL;
    
    dev->realdevice = ddev;
    
    dev->typecode = TYPECODE_DISK;
    /* name & irq should be set by caller after creation */
    memcpy(dev->vendor_string, "<NONAME>", 8);
    dev->irq = IRQ_DISK;
    dev->io_length = IOLENGTH_DISK;
    
    dev->io_write = &disk_io_write;
    dev->io_read  = &disk_io_read;
    dev->update   = &disk_update;
    
    return dev;
}

void disk_destroy(device_t *disk_dev) {
    if (((diskdevice_t*)disk_dev->realdevice)->image != NULL)
	fclose(((diskdevice_t*)disk_dev->realdevice)->image);
    if (((diskdevice_t*)disk_dev->realdevice)->imagefilename != NULL)
	free(((diskdevice_t*)disk_dev->realdevice)->imagefilename);

    free(disk_dev->realdevice);
    free(disk_dev);
}

/* places the initialized data in disk 
 * returns:
 * 0 - OK
 * 1 - file too small to hold disk
 * 2 - unable to create file
 * 3 - invalid sectorsize/numsectors
 */
int disk_init(char *filename, int sectorsize, int numsectors,
              device_t *disk_dev) {
    diskdevice_t *disk;
    FILE *f;

    disk = (diskdevice_t *) disk_dev->realdevice;
    if (disk == NULL) return -1; 
    if (filename == NULL) {
	fprintf(stderr, "disk_init error: No filename for disk image\n");
	return -1;
    }
    if (numsectors < 10) return 3;
    if (sectorsize != 128 && sectorsize != 256 && sectorsize != 512 
        && sectorsize != 1024
	&& sectorsize != 2048 && sectorsize != 4096 && sectorsize != 8192
	&& sectorsize != 16384 && sectorsize != 32768 && sectorsize != 65536) {
	fprintf(stderr, "disk_init error: Invalid sector size (must be 2^n, n=7..16)\n");
	return 3;
    }

    disk->image = NULL;
    disk->imagefilename = NULL;
    disk->time_rot = 0;
    disk->time_fullseek = 0;
    disk->current_cylinder = 0;
    disk->numcylinders = 1;
    disk->numsectors = numsectors;
    disk->sectorsize = sectorsize;

    if ((f = fopen(filename, "r+b")) == NULL) { /* try open */
	void *zerobuf;
	int i;

	if (errno != ENOENT) {
	    perror("disk_init error: Cannot open image file");
	    return 2;
	}
	if ((f = fopen(filename, "w+b")) == NULL) { /* try create */
	    perror("disk_init error: Cannot create image file");
	    return 2;
	}

	/* zero the file */
	zerobuf = smalloc(sectorsize);
	memset(zerobuf, 0, sectorsize);
	for (i=0; i<numsectors; i++)
	    if (fwrite(zerobuf, sectorsize, 1, f) != 1) {
		fprintf(stderr, 
			"disk_init error: Cannot init image file\n");
		fclose(f);
		unlink(filename);
		return 2;
	    }
	fflush(f);
	free(zerobuf);
    } else { /* file exists */
	size_t fsize;
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	/* Allow bigger than necessary files, even though it may
	 * give unexpected results */
	if (sectorsize*numsectors > fsize) {
	    fprintf(stderr, "disk_init error: Image file too small to hold disk\n");
	    fclose(f);
	    return 1;
	}
	fseek(f, 0, SEEK_SET);
    }
    /* save the file "handle" */
    disk->image = f;
    disk->imagefilename = (char*)smalloc(strlen(filename) + 1);
    strcpy(disk->imagefilename, filename);

    disk->status_word = disk->data_port = 0;
    disk->transfer_sector = disk->transfer_addr = 0xffffffff;
    disk->new_transfer_sector = disk->new_transfer_addr = 0xffffffff;
    disk->next_interest = 0;
    disk->mode = MODE_IDLE;

    disk->irqcpu = 0;

    return 0;
}

/* returns: 0 - OK, 1 - invalid parameters, 2 - parameter already set 
 * value -1 for a parameter is ignored */
int disk_setparams(int numcylinders, int time_rot, int time_fullseek,
                   device_t *disk_dev) {
    int retval = 0;
    diskdevice_t *disk;

    disk = (diskdevice_t *) disk_dev->realdevice;

    if (disk == NULL) return -1;


    if (numcylinders > -1) {
	if (numcylinders == 0 || disk->numsectors % numcylinders) {
	    fprintf(stderr, "disk_setparams error: Number of cylinders does not divide number of sectors\n");
	    return 1;
	}
	if (disk->current_cylinder) {
	    fprintf(stderr, "disk_setparams error: too late to set numcylinders\n");
	    retval = 2;
	} else
	    disk->numcylinders = numcylinders;
    } 

    if (time_rot > -1) {
	disk->time_rot = MSEC2TICKS_64(time_rot);
    } 

    if (time_fullseek > -1) {
	disk->time_fullseek = MSEC2TICKS_64(time_fullseek);
    } 


    return retval;
}


/* This simulates the disk seek times very accurately 
 * The disk has no cylinder cache 
 */
static uint64_t calculate_seek_time(diskdevice_t *disk) {
    int64_t seektime, rtime;
    int tcyl, tsec;
    float csec;

    tcyl = /* target cylinder */
	disk->transfer_sector
	/ (disk->numsectors / disk->numcylinders);

    /* seek time to target cylinder */
    seektime = disk->current_cylinder - tcyl;
    if (seektime < 0) seektime = -seektime;
    seektime = 
	seektime
	* disk->time_fullseek
	/ (int64_t)disk->numcylinders;

    if (disk->time_rot == 0) /* avoid divby0 */
	return seektime;

    csec = /* current sector within cylinder */
	(float)(hardware->cycle_count % disk->time_rot)
	* (float)(disk->numsectors / disk->numcylinders)
	/ (float)disk->time_rot;
    tsec = /* target sector within cylinder */
	disk->transfer_sector 
	% (disk->numsectors / disk->numcylinders);

    if (csec > (float)tsec) /* tsec available in next rotation */
	tsec += (disk->numsectors / disk->numcylinders);

    /* rotation time to the beginning of the target sector */
    rtime =
	(int64_t)(((float)tsec - csec) * (float)disk->time_rot)
	/ (int64_t)(disk->numsectors / disk->numcylinders);


    if (seektime <= rtime)
	return rtime;

    /* calculate again */
    csec =
	(float)((hardware->cycle_count + seektime) % disk->time_rot)
	* (float)(disk->numsectors / disk->numcylinders)
	/ (float)disk->time_rot;
    tsec =
	disk->transfer_sector 
	% (disk->numsectors / disk->numcylinders);
    if (csec > (float)tsec)
	tsec += (disk->numsectors / disk->numcylinders);
    rtime =
	(int64_t)(((float)tsec - csec) * (float)disk->time_rot)
	/ (int64_t)(disk->numsectors / disk->numcylinders);

    return seektime + rtime;
}


int disk_io_write(device_t *dev, uint32_t addr, uint32_t data) {
    diskdevice_t *disk;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    if (addr >= IOLENGTH_DISK || (addr & 3)) 
	return 1; /* wrong address/alignment */

    disk = (diskdevice_t*)dev->realdevice;

    switch(addr) {
    case PORT_STATUS:
    case PORT_DATA:
	/* read-only ports */
	break;
    case PORT_COMMAND:
	disk->status_word &= STATUS_MASK; /* reset errors */
	switch(data) {
	case COMMAND_READ:
	    /* move parameters to the real place */
	    disk->transfer_sector = disk->new_transfer_sector;
	    disk->transfer_addr = disk->new_transfer_addr;

	    /* Check parameter values */
	    if (disk->transfer_sector >= disk->numsectors)
		disk->status_word |= ERROR_ISECT;

	    if (disk->transfer_addr + disk->sectorsize
		> hardware->memory->num_pages * hardware->memory->pagesize)
		disk->status_word |= ERROR_IADDR;

	    /* check disk status */
	    if (disk->mode != MODE_IDLE)
		disk->status_word |= ERROR_EBUSY;
	    if (disk->status_word & STATUS_IRQMASK) /* unhandled IRQs */
		disk->status_word |= ERROR_EBUSY;

	    /* don't do anything if errors set */
	    if (disk->status_word & ERROR_MASK) break;

	    /* mark busy and schedule the actual transfer time */
	    disk->status_word |= STATUS_RBUSY;
	    disk->mode = MODE_READING1;
	    disk->next_interest = hardware->cycle_count 
		+ calculate_seek_time(disk);

	    /* adjust current cylinder to the new one */
	    disk->current_cylinder =
		disk->transfer_sector
		/ (disk->numsectors / disk->numcylinders);

	    break;
	case COMMAND_WRITE:
	    /* move parameters to the real place */
	    disk->transfer_sector = disk->new_transfer_sector;
	    disk->transfer_addr = disk->new_transfer_addr;

	    /* Check parameter values */
	    if (disk->transfer_sector >= disk->numsectors)
		disk->status_word |= ERROR_ISECT;

	    if (disk->transfer_addr + disk->sectorsize
		> hardware->memory->num_pages * hardware->memory->pagesize)
		disk->status_word |= ERROR_IADDR;

	    /* check disk status */
	    if (disk->mode != MODE_IDLE)
		disk->status_word |= ERROR_EBUSY;
	    if (disk->status_word & STATUS_IRQMASK) /* unhandled IRQs */
		disk->status_word |= ERROR_EBUSY;

	    /* don't do anything if errors set */
	    if (disk->status_word & ERROR_MASK) break;

	    /* mark busy and schedule the actual transfer time */
	    disk->status_word |= STATUS_WBUSY;
	    disk->mode = MODE_WRITING1;
	    disk->next_interest = hardware->cycle_count 
		+ calculate_seek_time(disk);

	    /* adjust current cylinder to the new one */
	    disk->current_cylinder =
		disk->transfer_sector
		/ (disk->numsectors / disk->numcylinders);

	    break;
	case COMMAND_RESETRIRQ:
	    disk->status_word &= ~STATUS_RIRQ;
	    break;
	case COMMAND_RESETWIRQ:
	    disk->status_word &= ~STATUS_WIRQ;
	    break;
	case COMMAND_GETSEC:
	    disk->data_port = disk->numsectors;
	    break;
	case COMMAND_GETSECSIZE:
	    disk->data_port = disk->sectorsize;
	    break;
	case COMMAND_GETSECPERCYL:
	    disk->data_port =
		disk->numsectors / disk->numcylinders;
	    break;
	case COMMAND_GETROT:
	    disk->data_port = TICKS2MSEC_32(disk->time_rot);
	    break;
	case COMMAND_GETSEEK:
	    disk->data_port = TICKS2MSEC_32(disk->time_fullseek);
	    break;
	default:
	    disk->status_word |= ERROR_ICOMM;
	}
	break;
    case PORT_TSECTOR:
	disk->new_transfer_sector = data;
	break;
    case PORT_DMAADDR:
	disk->new_transfer_addr = data;
	break;
    }
    
    return 0;
}

int disk_io_read(device_t *dev, uint32_t addr, uint32_t *data) {
    diskdevice_t *disk;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    if (addr >= IOLENGTH_DISK || (addr & 3)) 
	return 1; /* wrong address/alignment */

    disk = (diskdevice_t*)dev->realdevice;

    switch(addr) {
    case PORT_STATUS:
	*data = disk->status_word;
	break;
    case PORT_COMMAND:
	*data = 0; /* command is "write-only" */
	break;
    case PORT_DATA:
	*data = disk->data_port;
	break;
    case PORT_TSECTOR:
	*data = disk->new_transfer_sector;
	break;
    case PORT_DMAADDR:
	*data = disk->new_transfer_addr;
	break;
    }

    return 0;
}

int disk_update(device_t *dev) {
    diskdevice_t *disk;
    if (dev == NULL || dev->realdevice == NULL) return -1; /* fatal */
    disk = (diskdevice_t*)dev->realdevice;

    /* raise the IRQ line on every cycle */
    if (disk->status_word & STATUS_IRQMASK)
	RAISE_HW_INTERRUPT(disk->irqcpu, dev->irq);

    /* do we need to do anything? */
    if (disk->next_interest != hardware->cycle_count 
	|| disk->mode == MODE_IDLE)
	return 0;

    switch(disk->mode) {
    case MODE_READING1: /* read seek complete */
	{
	    size_t result = 0;

	    /* do the actual transfer */
	    fseek(disk->image,
		  disk->sectorsize * disk->transfer_sector,
		  SEEK_SET);
	    result = fread((char*)(hardware->memory->physmem)
			           + disk->transfer_addr,
			   disk->sectorsize,
			   1,
			   disk->image);
	    fflush(disk->image);
	    if (result != 1) {
		fprintf(stderr, "Error readind from disk\n");
		assert(result == 1);
	    }
	}

	/* this is how long one sector read/write takes */
	disk->next_interest = hardware->cycle_count + 
	    disk->time_rot
	    / (disk->numsectors / disk->numcylinders);
	if (disk->next_interest == hardware->cycle_count)
	    disk->next_interest++;
	disk->mode = MODE_READING2;

	break;
    case MODE_READING2: /* read transfer complete */
	/* clear busy flag and start yelling at the IRQ line */
	disk->status_word &= ~STATUS_RBUSY;
	/* select a CPU for the IRQ, unless IRQ already active */
	if (!(disk->status_word & STATUS_MASK))
	    disk->irqcpu = select_cpu_for_irq();
	disk->status_word |= STATUS_RIRQ;
	disk->mode = MODE_IDLE;
	disk->next_interest = 0;
	break;
    case MODE_WRITING1: /* write seek complete */
	{
	    size_t result = 0;

	    /* do the actual transfer */
	    fseek(disk->image,
		  disk->sectorsize * disk->transfer_sector,
		  SEEK_SET);
	    result = fwrite((char*)(hardware->memory->physmem)
			            + disk->transfer_addr,
			    disk->sectorsize,
			    1,
			    disk->image);
	    fflush(disk->image);
	    if (result != 1) {
		fprintf(stderr, "Error writing to disk\n");
		assert(result == 1);
	    }
	}

	/* this is how long one sector read/write takes */
	disk->next_interest = hardware->cycle_count + 
	    disk->time_rot
	    / (disk->numsectors / disk->numcylinders);
	if (disk->next_interest == hardware->cycle_count)
	    disk->next_interest++;
	disk->mode = MODE_WRITING2;

	break;
    case MODE_WRITING2: /* write transfer complete */
	/* clear busy flag and start yelling at the IRQ line */
	disk->status_word &= ~STATUS_WBUSY;
	/* select a CPU for the IRQ, unless IRQ already active */
	if (!(disk->status_word & STATUS_MASK))
	    disk->irqcpu = select_cpu_for_irq();
	disk->status_word |= STATUS_WIRQ;
	disk->mode = MODE_IDLE;
	disk->next_interest = 0;
	break;
    default:
	return -1; /* unknown mode */
    }

    return 0;
}
