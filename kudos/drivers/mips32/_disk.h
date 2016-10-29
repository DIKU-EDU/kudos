/*
 * Disk driver
 */

#ifndef KUDOS_DRIVERS_MIPS32__DISK_H
#define KUDOS_DRIVERS_MIPS32__DISK_H

#define DISK_COMMAND_READ            0x1
#define DISK_COMMAND_WRITE           0x2 
#define DISK_COMMAND_RIRQ            0x3
#define DISK_COMMAND_WIRQ            0x4  
#define DISK_COMMAND_BLOCKS          0x5
#define DISK_COMMAND_BLOCKSIZE       0x6
#define DISK_COMMAND_BLOCKSPERCYL    0x7
#define DISK_COMMAND_ROTTIME         0x8
#define DISK_COMMAND_SEEKTIME        0x9

#define DISK_STATUS_RBUSY(status)  ((status) & 0x00000001)
#define DISK_STATUS_WBUSY(status)  ((status) & 0x00000002)
#define DISK_STATUS_RIRQ(status)   ((status) & 0x00000004)
#define DISK_STATUS_WIRQ(status)   ((status) & 0x00000008) 


#define DISK_STATUS_ISECT(status)  ((status) & 0x08000000) 
#define DISK_STATUS_IADDR(status)  ((status) & 0x10000000) 
#define DISK_STATUS_ICOMM(status)  ((status) & 0x20000000) 
#define DISK_STATUS_EBUSY(status)  ((status) & 0x40000000) 
#define DISK_STATUS_ERROR(status)  ((status) & 0x80000000) 

#define DISK_STATUS_ERRORS(status) ((status) & 0xf8000000)


/* Structure of YAMS disk io area. */
typedef struct {
    volatile uint32_t status;
    volatile uint32_t command;
    volatile uint32_t data;
    volatile uint32_t tsector;
    volatile uint32_t dmaaddr;
} disk_io_area_t;


#endif // KUDOS_DRIVERS_MIPS32__DISK_H
