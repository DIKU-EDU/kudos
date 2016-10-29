/*
 * YAMS specific constants
 */

#ifndef KUDOS_DRIVERS_MIPS32_ARCH_H
#define KUDOS_DRIVERS_MIPS32_ARCH_H

#include "lib/types.h"

/* Start address of KUDOS kernel. */
#define KERNEL_BOOT_ADDRESS 0x80010000

/* Start address of YAMS IO descriptor area. */
#define IO_DESCRIPTOR_AREA 0xb0000000

/* The maximum number of devices supported by YAMS. */
#define YAMS_MAX_DEVICES 128

/* Start address of YAMS boot parameter area. */
#define BOOT_ARGUMENT_AREA 0xb0001000

/* Page size */
#define PAGE_SIZE 4096

/* Page portion of any address */
#define PAGE_SIZE_MASK 0xfffff000

/* Offset into page of any address */
#define PAGE_OFFSET_MASK (~PAGE_SIZE_MASK)

/* Ensure that the srtuctures are correctly packed into memory */

/* The structure of YAMS IO descriptor. */
typedef struct {
    /* Type of the device */
    uint32_t type          __attribute__ ((packed)); 

    /* Start address of the device io base */
    uint32_t io_area_base  __attribute__ ((packed));     

    /* Length of the device io base */
    uint32_t io_area_len   __attribute__ ((packed));

    /* The interrupt line used by the device */
    uint32_t irq           __attribute__ ((packed));

     /* Vendor string of the device (Note: This 
        is NOT null terminated! */
    char vendor_string[8];

    /* Reserved area (unused). */
    uint32_t resv[2]       __attribute__ ((packed));
} io_descriptor_t;


#define YAMS_TYPECODE_RTC 0x102
#define YAMS_TYPECODE_MEMINFO 0x101
#define YAMS_TYPECODE_SHUTDOWN 0x103
#define YAMS_TYPECODE_CPUSTATUS 0xc00
#define YAMS_TYPECODE_CPUMASK 0xffffff00

#endif // KUDOS_DRIVERS_MIPS32_ARCH_H
