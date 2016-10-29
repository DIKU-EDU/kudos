/*
 * Disk driver
 */

#ifndef KUDOS_DRIVERS_DISK_H
#define KUDOS_DRIVERS_DISK_H

#include <arch.h>
#include <_disk.h>
#include "lib/libc.h"
#include "kernel/spinlock.h"
#include "kernel/semaphore.h"
#include "drivers/device.h"
#include "drivers/gbd.h"

/* Internal data structure for disk driver. */
typedef struct {
    /* spinlock for synchronization of access to this data structure. */
    spinlock_t                 slock;

    /* Queue of pending requests. New requests are placed to queue
       by disk scheduling policy (see disksched_schedule()). */
    volatile gbd_request_t     *request_queue;

    /* Request currently served by the driver. If NULL device is idle. */
    volatile gbd_request_t     *request_served;
} disk_real_device_t;


/* functions */
int disk_init(io_descriptor_t *desc);


#endif // KUDOS_DRIVERS_DISK_H
