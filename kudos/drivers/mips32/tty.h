/*
 * TTY driver
 */

#ifndef KUDOS_DRIVERS_MIPS32_TTY_H
#define KUDOS_DRIVERS_MIPS32_TTY_H

#include <arch.h>
#include "kernel/spinlock.h"
#include "drivers/gcd.h"

/* The structure of the YAMS TTY IO area */
typedef struct {
    uint32_t status;    /* Status port of the TTY device. */
    uint32_t command;   /* Command port of the TTY device */
    uint32_t data;      /* Data port of the TTY device */
} tty_io_area_t;

/* Macros to get the fields from the TTY status port. */
#define TTY_STATUS_RAVAIL(status) \
    ((status) & 0x00000001)

#define TTY_STATUS_WBUSY(status) \
    (((status) & 0x00000002) >> 1)

#define TTY_STATUS_RIRQ(status) \
    (((status) & 0x00000004) >> 2)

#define TTY_STATUS_WIRQ(status) \
    (((status) & 0x00000008) >> 3)

#define TTY_STATUS_WIRQE(status) \
    (((status) & 0x00000010) >> 4)

#define TTY_STATUS_ICOMM(status) \
    (((status) & 0x20000000) >> 29)

#define TTY_STATUS_EBUSY(status) \
    (((status) & 0x40000000) >> 30)

#define TTY_STATUS_ERROR(status) \
    (((status) & 0x80000000) >> 31)

/* TTY commands */
#define TTY_COMMAND_RIRQ 0x01
#define TTY_COMMAND_WIRQ 0x02
#define TTY_COMMAND_WIRQE 0x03
#define TTY_COMMAND_WIRQD 0x04

#define TTY_BUF_SIZE   2048     /* Size of TTY's internal buffer */

/* TTY's real_device data structure. Structure of this type is stored
   the real_device field of device_t data structure. It contains
   internal buffers and synchronization for one TTY. Buffers are
   needed so that more than one character may be written to TTY device
   in one interrupt.*/
typedef struct {
    /* Spinlock to synchronize acccess to buffers. Only one spinlock
       because device can't read and write simultaneously. */
    spinlock_t *slock;

    char read_buf[TTY_BUF_SIZE];  /* read buffer */
    int read_head;                /* index to the beginning of data */
    int read_count;               /* number of chars in buffers */

    char write_buf[TTY_BUF_SIZE]; /* write buffer */
    int write_head;               /* index to the beginning of data */
    int write_count;              /* number of chars in buffers */
} tty_real_device_t;


device_t *tty_init(io_descriptor_t *desc);
void tty_interrupt_handle(device_t *device);

#endif // KUDOS_DRIVERS_MIPS32_TTY_H
