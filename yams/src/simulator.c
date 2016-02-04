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

   $Id: simulator.c,v 1.43 2010/11/24 08:54:15 tlilja Exp $

   $Log: simulator.c,v $
   Revision 1.43  2010/11/24 08:54:15  tlilja
   Reworked the GDB serial line interface support
    - drooped hw console support
    - made gdb.c use async_input interface
    - added documentation to info

   Revision 1.42  2010/11/23 12:20:26  jaatroko
   support for little-endian simulator

   Revision 1.41  2005/06/05 15:00:22  jaatroko
   C99 constant and formatting macros

   Revision 1.40  2005/06/05 14:20:55  jaatroko
   support for MMAP areas

   Revision 1.39  2005/04/16 11:03:21  jaatroko
   Removed all references to alloca() since it was not used

   Revision 1.38  2005/04/15 09:14:49  jaatroko
   check return value of async_input_start()

   Revision 1.37  2005/04/14 17:48:22  jaatroko
   Moved to use async_input + signal handler bugfix

   Revision 1.36  2005/03/08 12:57:01  lsalmela
   Inter-cpu interrupts

   Revision 1.35  2002/12/03 19:59:54  ttakanen
   *** empty log message ***

   Revision 1.34  2002/11/22 21:11:04  ttakanen
   Excluded prompt printing when readline is in use.

   Revision 1.33  2002/11/21 10:32:28  ttakanen
   Fixed breakpoints in exception handler entrys

   Revision 1.32  2002/11/21 00:42:50  ttakanen
   *** empty log message ***

   Revision 1.31  2002/11/20 20:33:28  tlilja
   Fixed some void * arithmetic stuff

   Revision 1.30  2002/11/20 13:03:32  ttakanen
   Removed old XXXs

   Revision 1.29  2002/11/20 12:44:32  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.28  2002/11/17 15:21:11  ttakanen
   *** empty log message ***

   Revision 1.27  2002/11/17 14:59:13  ttakanen
   *** empty log message ***

   Revision 1.26  2002/11/11 20:20:57  javirta2
   simulator_run(): loop until harware->running != 1

   Revision 1.25  2002/11/11 19:56:33  javirta2
   *** empty log message ***

   Revision 1.24  2002/11/11 15:28:33  tlilja
   Updated copyright notices

   Revision 1.23  2002/11/11 13:32:06  lsalmela
   count register fix

   Revision 1.22  2002/11/08 19:18:49  javirta2
   Meta io devices implemented and tested.

   Revision 1.21  2002/11/06 12:52:53  ttakanen
   Harmonized IO device headers

   Revision 1.20  2002/11/06 08:02:32  javirta2
   Fixed memory lookup vectors and mem_read. RTC works.

   Revision 1.19  2002/11/05 21:09:16  javirta2
   Shutdown device.

   Revision 1.18  2002/11/05 10:55:08  ttakanen
   Added io descriptor area creation

   Revision 1.17  2002/11/04 21:29:32  javirta2
   Added RTC-device. Compile with 'ACMEDEVICES' defined.

   Revision 1.16  2002/10/26 15:00:41  ttakanen
   TLB structures and virtual address mapping

   Revision 1.15  2002/10/10 16:52:32  tlilja
   *** empty log message ***

   Revision 1.14  2002/10/09 08:57:42  tlilja
   Fixed bug in simulator.c
   Code works now in Linux and Solaris.

   Revision 1.13  2002/10/08 17:56:36  tlilja
   *** empty log message ***

   Revision 1.12  2002/10/05 11:41:20  ttakanen
   Added hardware device updating in the main simulator loop.

   Revision 1.11  2002/10/05 11:33:14  ttakanen
   Implemented device lookup vectors for memory mapped devices.

   Revision 1.10  2002/10/04 20:43:58  ttakanen
   *** empty log message ***

   Revision 1.9  2002/10/04 17:50:05  ttakanen
   Added startup prints on hardware configuration.

   Revision 1.8  2002/10/04 17:08:12  ttakanen
   Implemented simulator_add_device

   Revision 1.7  2002/10/03 15:54:19  ttakanen
   Added unbreak-command

   Revision 1.6  2002/10/03 12:16:00  javirta2
   *** empty log message ***

   Revision 1.5  2002/10/03 11:26:00  ttakanen
   Hardware console implementation skeleton works.

   Revision 1.4  2002/10/02 15:52:18  ttakanen
   Added struct names for struct typedefs.

   Revision 1.3  2002/10/01 18:17:48  lsalmela
   *** empty log message ***

   Revision 1.2  2002/10/01 12:28:32  ttakanen
   Made simulator.c compilable.

   Revision 1.1  2002/09/29 16:20:55  ttakanen
   Initial version


*/

/*
  Simulator engine core.
*/

#include <assert.h>
#include "includes.h"
#include <signal.h>
#include <stdio.h>
#include "simulator.h"
#include "hwconsole.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include "trivial_io.h"
#include "async_input.h"
#include "gdb.h"

static volatile int sigint_caught = 0;
hardware_t *hardware = NULL;

/* Whether to simulate a big-endian or a little-endian machine: */
int simulator_bigendian = 1;


/* This adds onboard metadevices:
   - Software shutdown device
   - RTC
   - cpu info device(s)
   - memory info device
 */
static void simulator_add_metadevices(int cpu_irq) {
    int i;

    simulator_add_device(shutdown_create());
    simulator_add_device(rtc_create());
    simulator_add_device(meminfo_create());

    /* one device for each processor */
    for(i=0;i<hardware->num_cpus;i++) {
	simulator_add_device(cpuinfo_create(i));
    }
    set_cpu_irq(cpu_irq);
}

void simulator_create(int mem_pages, int num_cpus, int clock_speed,
                      int cpu_irq) {
    int i;

    hardware_t *hw;

    hw = (hardware_t *) smalloc(sizeof(hardware_t));
    hw->memory      = memory_create(SIMULATOR_PAGESIZE, mem_pages);

    /* cpus */
    hw->num_cpus    = num_cpus;
    hw->cpus        = (cpu_t **) smalloc(sizeof(cpu_t *) * num_cpus);
    for (i=0;i<num_cpus;i++) {
	hw->cpus[i] = cpu_init(i);
    }
    hw->clockspeed  = clock_speed;

    /* Devices are created later by configuration system. 
       See simulator_add_device. */
    hw->devices     = NULL;
    hw->num_devices = 0;

    hw->cycle_count   = 0;
    hw->running       = 0;

    /* non-reachable PC address */
    hw->breakpoint    = 0xffffffff;

    hardware = hw;

    /* add metadevices */
    simulator_add_metadevices(cpu_irq);
}

void simulator_add_device(device_t *dev) {
    uint32_t iobase = IO_AREA_BASE_ADDRESS;

    assert(dev != NULL);
    assert(dev->io_length % 4 == 0);

   /* Add dev to hw descriptor list*/

    if(hardware->devices != NULL) {
	iobase = hardware->devices->io_base + 
	    hardware->devices->io_length;
    }
    dev->next = hardware->devices;
    hardware->devices = dev;
    hardware->num_devices++;

    /* Allocate memory mapped IO-region */

    dev->io_base = iobase;
}

void simulator_add_mmap(pluggable_device_t *dev) {
    assert(dev != NULL);

    dev->mmap_next = hardware->memory->mmap;
    hardware->memory->mmap = dev;
    hardware->memory->num_mmap++;
}

static void simulator_print_hw_info() {
    device_t *dev;
    pluggable_device_t *pdev;

    printf("Simulated hardware:\n");
    printf(" %d CPU(s) with virtual clock rate %" PRId32 " Hz\n", 
	   hardware->num_cpus,
	   hardware->clockspeed);
    printf(" %u kilobytes of main memory (%u x %u)\n",
	   hardware->memory->pagesize * hardware->memory->num_pages / 1024,
	   hardware->memory->num_pages,
	   hardware->memory->pagesize);

    printf(" %d other virtual device(s): \n", hardware->num_devices);

    for(dev=hardware->devices; dev != NULL; dev = dev->next) {
	printf("   - '%c%c%c%c%c%c%c%c' Type: #%.8" PRIx32
	       " IOBASE: #%.8" PRIx32 " IRQ: ",
	       dev->vendor_string[0], dev->vendor_string[1],
	       dev->vendor_string[2], dev->vendor_string[3],
	       dev->vendor_string[4], dev->vendor_string[5],
	       dev->vendor_string[6], dev->vendor_string[7],
	       dev->typecode, dev->io_base);

	if(dev->irq < 0 || dev->irq > 5)
	    printf("NONE\n");
	else
	    printf("%" PRId32 "\n", dev->irq);
    }

    if (hardware->memory->num_mmap > 0)
	printf(" %d additional memory mapped I/O area(s):\n",
	       hardware->memory->num_mmap);

    for (pdev=hardware->memory->mmap; pdev != NULL; pdev = pdev->mmap_next) {
	printf("   - Base: #%.8" PRIx32 " Size: #%.8" PRIx32 "\n",
	       pdev->mmap_base, pdev->mmap_size);
    }


    printf("\n");
}

void simulator_init() {
    device_t *dev;
    pluggable_device_t *pdev;
    int io_length=0;
    int i, offset=0;
    uint32_t base;

    if(hardware->devices != NULL) {
	io_length = hardware->devices->io_base -
	    IO_AREA_BASE_ADDRESS +
	    hardware->devices->io_length;
    }
    
    hardware->memory->io_area_base   = IO_AREA_BASE_ADDRESS;
    hardware->memory->io_area_length = io_length;

    /* Allocate lookup vectors for memory mapped devices */
    hardware->memory->io_devices =
	(device_t **) smalloc(sizeof(device_t *) * (io_length/4));
    hardware->memory->io_device_write =
	(int (**)(device_t *dev, uint32_t addr, uint32_t data))
	smalloc(sizeof(int (*)()) * (io_length/4));
    hardware->memory->io_device_read =
	(int (**)(device_t *dev, uint32_t addr, uint32_t *data))
	smalloc(sizeof(int (*)()) * (io_length/4));

    /* Set up just allocated device lookup vectors for MMU */
    offset = io_length/4 - 1;
    for(dev=hardware->devices; dev != NULL; dev = dev->next) {
	for(i=0; i<dev->io_length; i+=4) {
	    hardware->memory->io_devices[offset]         = dev;
	    hardware->memory->io_device_write[offset]    = dev->io_write;
	    hardware->memory->io_device_read[offset]     = dev->io_read;
	    offset -= 1;
	}
    }

    /* Allocate the MMAP areas: */
    base = IO_AREA_BASE_ADDRESS + io_length;
    for (pdev=hardware->memory->mmap; pdev != NULL; pdev = pdev->next) {
	/* page align: */
	base = (base + SIMULATOR_PAGESIZE - 1) & ~(SIMULATOR_PAGESIZE - 1);

	if (base + pdev->mmap_size >= 0xc0000000) {
	    printf("Not enough I/O memory left for memory mapped I/O area\n");
	    exit(1);
	}

	if (pdev->mmap_size > 0)
	    plugio_mmap(pdev, base);
	base += pdev->mmap_size;
    }
    
    /* Set up device descriptor vector at 0xB0000000 */
    offset = 0;
    for(dev=hardware->devices; dev != NULL; dev = dev->next) {
	uint32_t temp;

	temp = htosim32(dev->typecode);
	memcpy((uint8_t *)hardware->memory->io_descrarea + offset + 0x00,
               &temp, 4);

	temp = htosim32(dev->io_base);
	memcpy((uint8_t *)hardware->memory->io_descrarea + offset + 0x04,
               &temp, 4);

	temp = htosim32(dev->io_length);
	memcpy((uint8_t *)hardware->memory->io_descrarea + offset + 0x08,
               &temp, 4);

	temp = htosim32(dev->irq);
	memcpy((uint8_t *)hardware->memory->io_descrarea + offset + 0x0c,
               &temp, 4);

	memcpy((uint8_t *)hardware->memory->io_descrarea + offset + 0x10,
	       dev->vendor_string, 8);

	/* last two words are left unassigned by specification */

	offset += 32; /* descriptors are 32 bytes long */
    }

    /* start the input poller */
    if (async_input_start())
	exit(1);

    simulator_print_hw_info();
}

/* Used to break out of simulation loop in simulator_run and re-enter
   to hwconsole. 
*/

static void sigint_handler(int signum) {
    signum = signum;
    sigint_caught = 1;
}


void simulator_run(uint64_t stop_at_cycle) {
    int i;
    device_t *dev;
    void *old_sigint;

    sigint_caught = 0;
    hardware->running = 1;
    
    old_sigint = signal(SIGINT, sigint_handler);
    while(hardware->running == 1 && stop_at_cycle != hardware->cycle_count &&
	  !sigint_caught) {

	/* advance one cycle for each cpu */
	for(i=0;i<hardware->num_cpus;i++) {
	    cpu_update(hardware->cpus[i]);
	}

	async_input_lock();

        if (gdb_interface_check_and_run() == 0)
            /* we halt the simulation if the gdb interface indicated so */
            hardware->running = -1;

	/* update all hardware */
	for(dev=hardware->devices; dev != NULL; dev = dev->next) {
	    if(dev->update != NULL)
		(*(dev->update))(dev);
	}

	async_input_unlock();

	/* All done */
	hardware->cycle_count++;
    }

    signal(SIGINT, old_sigint);
}
